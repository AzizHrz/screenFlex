// ModeUtils.cpp
#include "utils/ModeUtils.h"
#include "utils/Mylog.h"
#include "dataModel.h"
#include "utils/Persistence.h"
#include "utils/ScheduleUtils.h"
#include "network/NetworkUtils.h"

extern ModeData currentMode;
extern ScheduleData currentSchedule;
extern TVState desiredTVState;
extern String DEVICE_ID;
extern bool useEthernet;
extern IPAddress serverIp;
extern int serverPort;
extern WiFiClient client;

void updateDesiredState() {
    if (currentMode.useSchedule) {
        desiredTVState = checkSchedule(currentSchedule) ? TV_ON : TV_OFF;
    }
}

bool fetchModeFromServer() {
    if (!connectToNetwork()) {
        MyLog(INFO, "Not connected - skipping mode update");
        return false;
    }

    HTTPClient http;
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/api/v1/esp/" + DEVICE_ID + "/mode";

    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, "/api/v1/esp/" + DEVICE_ID + "/mode");
    } else {
        http.begin(client, url);
    }

    http.setTimeout(5000);
    http.setReuse(true);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        MyLogF(ERROR, "Mode check failed, HTTP code: %d", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        MyLogF(ERROR, "Failed to parse mode response: %s", error.c_str());
        return false;
    }

    if (!doc["success"]) {
        MyLogF(ERROR, "Server error: %s", doc["error"].as<String>().c_str());
        return false;
    }

    bool newMode = doc["data"]["use_schedule"];
    if (newMode != currentMode.useSchedule) {
        currentMode.useSchedule = newMode;
        saveMode(currentMode);
        MyLogF(INFO, "Mode updated to: %s", currentMode.useSchedule ? "schedule" : "server commands");
        updateDesiredState();
        return true;
    }

    return false;
}
