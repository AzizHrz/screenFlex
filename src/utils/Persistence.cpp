#include "utils/Persistence.h"
#include "utils/Mylog.h"
#include "dataModel.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config/config.h"

// 1

extern PersistentData persistentData;
extern ModeData currentMode;
extern ScheduleData currentSchedule;
extern TVState desiredTVState;

bool savePersistentData() {
    if (!LittleFS.begin()) {
        MyLog(ERROR, "Failed to mount LittleFS");
        return false;
    }

    File file = LittleFS.open("/persistent.json", "w");
    if (!file) {
        MyLog(ERROR, "Failed to open persistent data file");
        return false;
    }

    DynamicJsonDocument doc(1024);
    doc["mode"]["use_schedule"] = persistentData.mode.useSchedule;
    doc["schedule"]["active"] = persistentData.schedule.active;
    doc["schedule"]["timezone"] = persistentData.schedule.timezone;
    doc["schedule"]["start_time"] = persistentData.schedule.startTime;
    doc["schedule"]["end_time"] = persistentData.schedule.endTime;

    JsonArray days = doc["schedule"].createNestedArray("weekdays");
    for (int i = 0; i < persistentData.schedule.daysCount; i++) {
        days.add(persistentData.schedule.days[i]);
    }

    doc["last_command"] = (int)persistentData.lastCommandedState;
    doc["last_update"] = persistentData.lastUpdate;

    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    if (bytesWritten == 0) {
        MyLog(ERROR, "Failed to write persistent data");
        return false;
    }

    MyLog(INFO, "Persistent data saved successfully");
    return true;
}

bool loadPersistentData() {
    if (!LittleFS.begin()) {
        MyLog(ERROR, "Failed to mount LittleFS");
        return false;
    }

    if (!LittleFS.exists("/persistent.json")) {
        MyLog(INFO, "No persistent data file, using defaults");
        return false;
    }

    File file = LittleFS.open("/persistent.json", "r");
    if (!file) {
        MyLog(ERROR, "Failed to open persistent data file");
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        MyLogF(ERROR, "Failed to parse persistent data: %s", error.c_str());
        file.close();
        return false;
    }

    persistentData.mode.useSchedule = doc["mode"]["use_schedule"];

    persistentData.schedule.active = doc["schedule"]["active"];
    persistentData.schedule.timezone = doc["schedule"]["timezone"].as<String>();
    persistentData.schedule.startTime = doc["schedule"]["start_time"].as<String>();
    persistentData.schedule.endTime = doc["schedule"]["end_time"].as<String>();

    persistentData.schedule.daysCount = 0;
    for (JsonVariant day : doc["schedule"]["weekdays"].as<JsonArray>()) {
        if (persistentData.schedule.daysCount < 7) {
            persistentData.schedule.days[persistentData.schedule.daysCount++] = day.as<String>();
        }
    }

    persistentData.lastCommandedState = (TVState)(int)doc["last_command"];
    persistentData.lastUpdate = doc["last_update"];

    file.close();

    currentMode = persistentData.mode;
    currentSchedule = persistentData.schedule;

    if (!currentMode.useSchedule) {
        desiredTVState = persistentData.lastCommandedState;
    }

    MyLog(INFO, "Persistent data loaded successfully");
    return true;
}

// 2

bool saveWiFiConfig(const WiFiConfig& config) {
    if (!LittleFS.begin()) {
        MyLog(ERROR, "Failed to mount LittleFS");
        return false;
    }

    File file = LittleFS.open(WIFI_CONFIG_FILE, "w");
    if (!file) {
        MyLog(ERROR, "Failed to open WiFi config file");
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
        MyLog(ERROR, "Failed to mount LittleFS");
        return false;
    }

    if (!LittleFS.exists(WIFI_CONFIG_FILE)) {
        MyLog(INFO, "No WiFi config file found");
        return false;
    }

    File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!file) {
        MyLog(ERROR, "Failed to open WiFi config file");
        return false;
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        MyLogF(ERROR, "Failed to parse WiFi config: %s", error.c_str());
        return false;
    }

    config.ssid = doc["ssid"].as<String>();
    config.password = doc["password"].as<String>();
    return true;
}

bool saveCurrentVersion(const String& version) {
    File file = LittleFS.open("/version.json", "w");
    if (!file) {
        MyLog(ERROR, "Failed to open version file for writing");
        return false;
    }

    DynamicJsonDocument doc(128);
    doc["version"] = version;
    serializeJson(doc, file);
    file.close();

    MyLogF(INFO, "Saved new firmware version: %s", version.c_str());
    return true;
}

String loadCurrentVersion() {
    if (!LittleFS.exists("/version.json")) {
        MyLog(INFO, "No version file found, creating one with default version");
        saveCurrentVersion(FIRMWARE_VERSION);
        return FIRMWARE_VERSION;
    }

    File file = LittleFS.open("/version.json", "r");
    if (!file) {
        MyLog(ERROR, "Failed to open version file");
        return FIRMWARE_VERSION;
    }

    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        MyLogF(ERROR, "Failed to parse version: %s", error.c_str());
        return FIRMWARE_VERSION;
    }

    return doc["version"].as<String>();
}

bool saveMode(const ModeData& mode) {
    if (!LittleFS.begin()) {
        MyLog(ERROR, "Failed to mount LittleFS");
        return false;
    }

    File file = LittleFS.open(MODE_FILE, "w");
    if (!file) {
        MyLog(ERROR, "Failed to open mode file for writing");
        return false;
    }

    DynamicJsonDocument doc(128);
    doc["use_schedule"] = mode.useSchedule;
    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    if (bytesWritten == 0) {
        MyLog(ERROR, "Failed to write mode");
        return false;
    }

    MyLogF(INFO, "Mode saved (%d bytes)", bytesWritten);
    return true;
}

bool loadMode(ModeData& mode) {
    if (!LittleFS.exists(MODE_FILE)) {
        MyLog(INFO, "No mode file, using default (schedule)");
        mode.useSchedule = true;
        return false;
    }

    File file = LittleFS.open(MODE_FILE, "r");
    if (!file) {
        MyLog(ERROR, "Failed to open mode file");
        mode.useSchedule = true;
        return false;
    }

    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        MyLogF(ERROR, "Failed to parse mode: %s", error.c_str());
        mode.useSchedule = true;
        return false;
    }

    mode.useSchedule = doc["use_schedule"];
    return true;
}

