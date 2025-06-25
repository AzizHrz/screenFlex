// newww module

#include "network/NetworkUtil.h"
#include "dataModel.h"
#include "network/network_stack.h"
#include "utils/Mylog.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Ethernet.h>

extern NetworkStack network;
extern String DEVICE_ID;
extern WiFiClient client;
extern IPAddress serverIp;
extern int serverPort;
extern String ssid;
extern String password;
extern bool useEthernet;
extern bool loadWiFiConfig(WiFiConfig& config);
extern uint8_t checkTVStatus();

bool connectToNetwork() {
    static WiFiConfig wifiConfig;
    static bool configLoaded = false;
    static unsigned long lastSwitch = 0;
    const unsigned long SWITCH_DELAY = 30000;

    if (!configLoaded) {
        if (!loadWiFiConfig(wifiConfig)) {
            wifiConfig.ssid = ssid;
            wifiConfig.password = password;
            MyLog(INFO, "Using hardcoded WiFi");
        } else {
            MyLog(INFO, "Loaded WiFi config from flash");
        }
        configLoaded = true;
    }

    if (millis() - lastSwitch < SWITCH_DELAY && lastSwitch != 0) {
        return useEthernet ? Ethernet.linkStatus() == LinkON : WiFi.status() == WL_CONNECTED;
    }

    if (!useEthernet) {
        if (initEthernet()) {
            useEthernet = true;
            lastSwitch = millis();
            return true;
        }
    }

    if (useEthernet && Ethernet.linkStatus() == LinkOFF) {
        MyLog(INFO, "Ethernet disconnected, switching to WiFi");
        useEthernet = false;
        network.configureWiFi(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        network.InitWifiModule();

        if (WiFi.status() != WL_CONNECTED) {
            MyLog(WARNING, "Saved WiFi failed, trying fallback credentials");
            network.configureWiFi(ssid, password);
            network.InitWifiModule();
        }
        lastSwitch = millis();
        return WiFi.status() == WL_CONNECTED;
    }

    if (!useEthernet && WiFi.status() != WL_CONNECTED) {
        network.configureWiFi(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        network.InitWifiModule();
        if (WiFi.status() != WL_CONNECTED) {
            network.configureWiFi(ssid, password);
            network.InitWifiModule();
        }
        lastSwitch = millis();
    }

    return useEthernet ? Ethernet.linkStatus() == LinkON : WiFi.status() == WL_CONNECTED;
}

void reportTVStateToServer() {
    if (!connectToNetwork()) {
        MyLog(INFO, "Not connected - skipping report");
        return;
    }

    uint8_t state = checkTVStatus();
    String tvState = state ? "off" : "on";
    String path = "/api/v1/esp/" + DEVICE_ID + "/state";

    HTTPClient http;
    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, path);
    } else {
        http.begin(client, "http://" + serverIp.toString() + ":" + String(serverPort) + path);
    }

    http.addHeader("Content-Type", "application/json");
    String postData = "{\"actual_state\":\"" + tvState + "\"}";
    MyLogF(INFO, "Reporting state: %s", tvState.c_str());

    int httpCode = http.POST(postData);
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        MyLogF(INFO, "Server response: %s", payload.c_str());

        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, payload);
        if (!err && doc["success"]) {
            MyLog(INFO, "Server acknowledged");
        }
    } else {
        MyLogF(ERROR, "POST failed: %d", httpCode);
        if (httpCode > 0) {
            String errorPayload = http.getString();
            MyLogF(ERROR, "Error: %s", errorPayload.c_str());
        }
    }

    http.end();
}
