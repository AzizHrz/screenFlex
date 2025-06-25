// i will create this newwww module  

// ScheduleUtils.cpp
#include "utils/ScheduleUtils.h"


// 2
#include "config/config.h"
#include "utils/Mylog.h"
#include "utils/Persistence.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

extern String DEVICE_ID;
extern bool useEthernet;
extern IPAddress serverIp;
extern int serverPort;
extern WiFiClient client;
extern ScheduleData defaultSchedule;

// 1

//==============================Done===================================
bool checkSchedule(const ScheduleData& schedule) {
    if (!schedule.active) {
        return false;
    }

    RtcDateTime now = rtc.RTC_getDateTime();
    String currentDay = getCurrentDay();

    
    // Check day match
    bool dayMatch = false;
    for (int i = 0; i < schedule.daysCount; i++) {
        if (currentDay.equalsIgnoreCase(schedule.days[i])) {
            dayMatch = true;
            break;
        }
    }
    if (!dayMatch) return false;

    // Parse times
    int startH, startM, endH, endM;
    if (sscanf(schedule.startTime.c_str(), "%d:%d", &startH, &startM) != 2) {
        MyLog(ERROR, "Invalid start time format");
        return false;
    }
    if (sscanf(schedule.endTime.c_str(), "%d:%d", &endH, &endM) != 2) {
        MyLog(ERROR, "Invalid end time format");
        return false;
    }

    int currentMins = now.Hour() * 60 + now.Minute();
    int startMins = startH * 60 + startM;
    int endMins = endH * 60 + endM;

    // Handle overnight schedules (end time < start time)
    if (endMins < startMins) {
        return (currentMins >= startMins) || (currentMins <= endMins);
    }
    
    return (currentMins >= startMins) && (currentMins <= endMins);
}

// Parses the schedule response from the server and updates the current schedule
bool parseScheduleResponse(const String& payload) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        MyLogF(ERROR, "JSON parse error: %s", error.c_str());
        return false;
    }

    if (!doc["success"]) {
        MyLogF(ERROR, "Server error: %s", doc["error"].as<String>().c_str());
        return false;
    }

    JsonObject scheduleObj = doc["data"]["schedule"];
    currentSchedule.active = scheduleObj["active"];
    currentSchedule.timezone = scheduleObj["timezone"].as<String>();
    currentSchedule.startTime = scheduleObj["start_time"].as<String>();
    currentSchedule.endTime = scheduleObj["end_time"].as<String>();
    currentSchedule.daysCount = 0;

    JsonArray daysArray = scheduleObj["weekdays"];
    for (JsonVariant day : daysArray) {
        if (currentSchedule.daysCount < 7) {
            currentSchedule.days[currentSchedule.daysCount++] = day.as<String>();
        }
    }

    // Save the new schedule to LittleFS
    if (saveSchedule(currentSchedule)) {
        MyLog(INFO, "Schedule saved to local storage");
        return true;
    } else {
        MyLog(ERROR, "Failed to save schedule locally");
        return false;
    }
}



bool isDaySelected(String day) {
    // Normalize the input
    String shortDay = day;
    for (int i = 0; i < 7; i++) {
        if (day.equalsIgnoreCase(longDays[i])) {
            shortDay = shortDays[i];
            break;
        }
    }

    // Check against current schedule
    for (int i = 0; i < currentSchedule.daysCount; i++) {
        if (currentSchedule.days[i].equalsIgnoreCase(shortDay)) {
            return true;
        }
    }
    return false;
}
