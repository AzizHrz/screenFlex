#include "network/network_stack.h"
#include "utils/Mylog.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "config/config.h"
#include "modules/RTC_module.h"
#include <Ethernet.h>
#include "esp_task_wdt.h"

IPAddress serverIP = SERVER_IP;
int ServerPort = SERVER_PORT; 

NetworkStack::NetworkStack() : MyMacAddr{0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}
{
    WoLRepetion = 10;
}
void NetworkStack::InitWifiModule(){
    #if (ENABLE_WIFI == 1)
    WiFi.begin(ssid, passwd);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
        delay(500);
        Serial.print(".");
        esp_task_wdt_reset();
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        LocalIP = WiFi.localIP();
    } else {
        Serial.println("Failed to connect to WiFi");
    }
    #endif
}
void NetworkStack::configureWiFi(const char *ssid, const char *passwd)
{
    #if (OTA_WIFI_ETH == 1)
    this->ssid = ssid;
    this->passwd = passwd;
    #endif
}

void NetworkStack::configureOTA(bool enable)
{
    isOTAEnabled = enable;
    MyLog(INFO, enable ? "OTA Enabled" : "OTA Disabled");
}

String NetworkStack::generateDeviceId() {
    uint8_t mac[6];
    #if (ENABLE_WIFI == 1)
        WiFi.macAddress(mac);
    #else
        Ethernet.MACAddress(mac);
    #endif
    
    char id[25]; // 24 chars + null terminator
    snprintf(id, sizeof(id), "%02X%02X%02X%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[0], mac[1], mac[2]);
    return String(id);
}

void NetworkStack::MacArrayToString(const uint8_t* arr, char* macString, size_t _sizeMacString)
{
    snprintf(macString, _sizeMacString, "%02X:%02X:%02X:%02X:%02X:%02X", 
             arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
}

String NetworkStack::HTTPGET(const String &reqPath)
{
    int max_retries = 3;
    int retry_delay = 1000;
    
    for (int i = 0; i < max_retries; i++) {
        if (client.connect(SERVER_IP, SERVER_PORT)) {
            client.printf("GET %s HTTP/1.1\r\n", reqPath.c_str());
            client.printf("Host: %s\r\n", String(SERVER_IP).c_str());
            client.println("Connection: close");
            client.println();

            // Wait for response
            unsigned long timeout = millis();
            while (!client.available() && millis() - timeout < 5000) {
                delay(10);
            }

            if (client.available()) {
                String response = client.readString();
                client.stop();
                return response;
            }
        }
        client.stop();
        delay(retry_delay);
    }
    Serial.println("[ERROR] Failed after 3 retries");
    return "";
}

String NetworkStack::HTTPPOST(const String &reqPath, const String &contentType, const String &data)
{
    String response = "";
    if (client.connect(serverIP, ServerPort))
    {
        client.println("POST " + reqPath + " HTTP/1.1");
        client.println("Host: " + serverIP.toString());
        client.println("Connection: close");
        client.println("Content-Type: " + contentType);
        client.printf("Content-Length: %d\r\n", data.length());
        client.println();
        client.println(data);
        response = ReadHTTPResponse();
        client.stop();
    }
    else
    {
        MyLog(ERROR, "Connection failed");
    }
    return response;
}

String NetworkStack::ReadHTTPResponse()
{
    String response;
    bool isHeader = true;

    while (client.connected() || client.available())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            isHeader = false;
        }
        else if (!isHeader) {
            response += line;
        }
    }
    return response;
}

const String NetworkStack::GetFirmwareVersion()
{
    String deviceId = generateDeviceId();
    String response = HTTPGET("/esp/" + deviceId + "/version");
    
    StaticJsonDocument<1024> doc; // Adjust size as needed
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        MyLogF(ERROR, "deserializeJson() failed: %s", error.c_str());
        return "";
    }
    
    if (doc["success"] == true) {
        return doc["data"]["version"].as<String>();
    }
    return "";
}

const String NetworkStack::GetSchedule()
{
    String response = this->HTTPGET("/schedule");
    StaticJsonDocument<1024> doc; // Adjust size as needed
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        MyLogF(ERROR, "deserializeJson() failed: %s", error.c_str());
        return "";
    }

    if (doc["success"]) {
        String beginTimeStr = doc["data"]["begin"].as<String>();
        String endTimeStr = doc["data"]["end"].as<String>();
        
        beginTime = rtc.convertToRtcDateTime(beginTimeStr);
        endTime = rtc.convertToRtcDateTime(endTimeStr);
        rtc.set_beginTime(beginTime);
        rtc.set_endTime(endTime);
        
        return beginTimeStr + " - " + endTimeStr;
    }
    return "";
}

bool NetworkStack::OTA()
{
    if (!isOTAEnabled) return false;
    
    const String newSoftV = GetFirmwareVersion();
    if (newSoftV != TEST_SOFTWARE_VERSION) {
        int FirmwareSize = RequestOTA("/update");
        if (FirmwareSize > 0) {
            return UpdateFirmware(FirmwareSize) > 0;
        }
    }
    return false;
}