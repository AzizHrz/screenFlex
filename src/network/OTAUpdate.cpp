// i will create this newwww module  

// OTAUpdate.cpp


#include "network/OTAUpdate.h"
#include "utils/Mylog.h"
#include "modules/RGBModule.h"
#include "config/config.h"
#include "dataModel.h"
#include "network/network_stack.h"
#include "Hardware.h"

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

extern WebServer otaServer;
extern bool useEthernet;
extern IPAddress serverIp;
extern int serverPort;
extern WiFiClient client;
extern String DEVICE_ID;
extern NetworkStack network;
extern String ssid;
extern String password;
extern bool loadWiFiConfig(WiFiConfig& config);
extern String loadCurrentVersion();
extern void saveCurrentVersion(const String& ver);
extern uint8_t checkTVStatus();

bool receivedNewSchedule = false;

// by me
extern RGBModule rgb;


bool performOTAUpdate(String firmwareUrl) {
    MyLog(INFO, "Starting OTA update...");
    rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);

    WiFiClient client;
    HTTPUpdateResult ret = httpUpdate.update(client, firmwareUrl);
    bool success = false;

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            MyLogF(ERROR, "OTA failed: %s", httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            MyLog(INFO, "No updates available");
            success = true;
            break;
        case HTTP_UPDATE_OK:
            MyLog(INFO, "OTA update completed");
            success = true;
            break;
    }

    rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
    return success;
}

void setupOTA() {
    if (!useEthernet) {
        if (!MDNS.begin(DEVICE_ID.c_str())) {
            MyLog(ERROR, "Error setting up MDNS responder!");
            while (1) delay(1000);
        } else {
            MyLog(INFO, "mDNS responder started");
            MDNS.addService("http", "tcp", 80);
        }
    }

    otaServer.on("/update", HTTP_GET, []() {
        otaServer.send(200, "text/html",
            "<form method='POST' action='/update' enctype='multipart/form-data'>"
            "<input type='file' name='update'>"
            "<input type='submit' value='Update'></form>");
    });

    otaServer.on("/update", HTTP_POST, []() {
        otaServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = otaServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            MyLog(INFO, "OTA Update Started");
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace)) {
                MyLog(ERROR, "OTA Update Begin Failed");
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                MyLog(ERROR, "OTA Write Failed");
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                MyLog(INFO, "OTA Update Success");
            } else {
                MyLog(ERROR, "OTA Update Failed");
            }
        }
    });

    otaServer.begin();
    MDNS.addService("http", "tcp", 80);
    MyLog(INFO, "OTA Update Server Ready");
}

bool checkForUpdates() {
    String currentVersion = loadCurrentVersion();
    MyLogF(INFO, "Current version: %s", currentVersion.c_str());

    HTTPClient http;
    String path = "/api/v1/esp/" + DEVICE_ID + "/firmware";
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + path;

    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, path);
    } else {
        http.begin(client, url);
    }

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc["success"]) {
            String latestVersion = doc["data"]["latest_version"].as<String>();
            bool updateAvailable = doc["data"]["update_available"].as<bool>();
            String firmwareUrl = doc["data"]["url"].as<String>();

            MyLogF(INFO, "Latest version: %s, Update available: %d",
                latestVersion.c_str(), updateAvailable);

            if (updateAvailable) {
                MyLog(INFO, "New version available, updating...");
                if (performOTAUpdate(firmwareUrl)) {
                    saveCurrentVersion(latestVersion);
                    ESP.restart();
                    return true;
                }
            } else {
                MyLog(INFO, "Firmware is up to date");
            }
        }
    } else {
        MyLogF(ERROR, "Update check failed: %d", httpCode);
    }

    http.end();
    return false;
}
