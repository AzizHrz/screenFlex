#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define WIFI_CONFIG_FILE "/wifi.json"

struct WiFiConfig {
  String ssid;
  String password;
};

bool saveWiFiConfig(const WiFiConfig& config) {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }

  File file = LittleFS.open(WIFI_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Failed to open WiFi config file for writing");
    return false;
  }

  DynamicJsonDocument doc(256);
  doc["ssid"] = config.ssid;
  doc["password"] = config.password;

  size_t bytesWritten = serializeJson(doc, file);
  file.close();
  return bytesWritten > 0;
}

bool loadWiFiConfig(WiFiConfig& config) {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }

  if (!LittleFS.exists(WIFI_CONFIG_FILE)) {
    Serial.println("No WiFi config file found");
    return false;
  }

  File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Failed to open WiFi config file for reading");
    return false;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.printf("Failed to parse WiFi config: %s\n", error.c_str());
    return false;
  }

  config.ssid = doc["ssid"].as<String>();
  config.password = doc["password"].as<String>();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFiConfig config;
  config.ssid = "MyTestSSID";
  config.password = "MyTestPassword";

  if (saveWiFiConfig(config)) {
    Serial.println("WiFi config saved!");
  }

  WiFiConfig loaded;
  if (loadWiFiConfig(loaded)) {
    Serial.println("WiFi config loaded!");
    Serial.printf("SSID: %s\n", loaded.ssid.c_str());
    Serial.printf("Password: %s\n", loaded.password.c_str());
  } else {
    Serial.println("Failed to load WiFi config.");
  }
}

void loop() {
  // Nothing
}
