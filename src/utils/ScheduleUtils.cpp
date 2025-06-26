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

//5
#include "RtcDateTime.h"
#include "RtcDS1307.h"
#include "modules/RTC_module.h"

#include "config/soft_version.h"
#include "Hardware.h"
#include "modules/RGBModule.h"
#include "modules/ButtonModule.h"
#include "modules/IR_Module.h"
#include "utils/Mylog.h"
#include <Arduino.h>
#include <RtcDateTime.h>
#include <LittleFS.h>
#include <time.h>
#include <WString.h>
#include <IRremote.h>

//6

#include <Arduino.h>
#include "network/network_stack.h"
#include "Hardware.h"

#include <HTTPClient.h>
#include <HTTPUpdate.h> // Include the ESP32 HTTP Update library
#include <ArduinoJson.h>
#include "RtcDateTime.h"
#include "RtcDS1307.h"
#include <LittleFS.h>
#include <time.h>
#include <TimeLib.h>
#include <Ethernet.h>
#include <SPI.h>

#include "network/NetworkUtils.h" // include for connectToNetwork
#include "dataModel.h"  // include for ScheduleData 
ScheduleData currentSchedule;

extern const char* shortDays[];
extern const char* longDays[] ;

#define IR_DATA_FILE "/ir_data.json" // unused
#define SCHEDULE_FILE "/schedule.json"
#define MODE_FILE "/mode.json"
#define VERSION_FILE "/version.json" // unused
#define WIFI_CONFIG_FILE "/wifi_config.json"



extern String DEVICE_ID;
extern bool useEthernet;
extern IPAddress serverIp;
extern int serverPort;
extern WiFiClient client;
extern ScheduleData defaultSchedule;

// 4

String getCurrentDay() {
    RtcDateTime rtcTime = rtc.RTC_getDateTime();
    
    // For DS1307, we need to read day of week directly from register
    uint8_t dayOfWeek;
    
    #ifdef RTC_DS1307
    Wire.beginTransmission(0x68);
    Wire.write(0x03); // Day of week register
    Wire.endTransmission();
    Wire.requestFrom(0x68, 1);
    if (Wire.available()) {
        dayOfWeek = Wire.read();
    } else {
        dayOfWeek = 0;
    }
    // DS1307 uses 1=Sunday through 7=Saturday
    dayOfWeek = (dayOfWeek - 1) % 7;
    #else
    // For other RTC chips that include day in DateTime
    dayOfWeek = rtcTime.DayOfWeek();
    #endif

    // Validate day
    if (dayOfWeek > 6) {
        // Calculate from date if invalid
        int d = rtcTime.Day();
        int m = rtcTime.Month();
        int y = rtcTime.Year();
        
        if (m < 3) {
            m += 12;
            y--;
        }
        
        dayOfWeek = (d + (13 * (m + 1) / 5) + y + (y / 4) - (y / 100) + (y / 400)) % 7;
    }

    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return days[dayOfWeek % 7];
}


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


// 2

bool saveSchedule(const ScheduleData& schedule) {
  if (!LittleFS.begin()) {
    MyLog(ERROR, "Failed to mount LittleFS");
    return false;
  }

  File file = LittleFS.open(SCHEDULE_FILE, "w");
  if (!file) {
    MyLog(ERROR, "Failed to open schedule file for writing");
    return false;
  }

  DynamicJsonDocument doc(1024);
  doc["active"] = schedule.active;
  doc["timezone"] = schedule.timezone;
  doc["start_time"] = schedule.startTime;
  doc["end_time"] = schedule.endTime;

  JsonArray daysArray = doc.createNestedArray("weekdays");
  for (int i = 0; i < schedule.daysCount; i++) {
    if (schedule.days[i].length() > 0) { // Only save non-empty days
      daysArray.add(schedule.days[i]);
    }
  }

  size_t bytesWritten = serializeJson(doc, file);
  file.close();

  if (bytesWritten == 0) {
    MyLog(ERROR, "Failed to write schedule");
    return false;
  }

  MyLogF(INFO, "Schedule saved (%d bytes)", bytesWritten);
  return true;
}

bool loadSchedule(ScheduleData& schedule) {
  if (!LittleFS.exists(SCHEDULE_FILE)) {
    MyLog(INFO, "No schedule file, using defaults");
    schedule = defaultSchedule;
    return false;
  }

  File file = LittleFS.open(SCHEDULE_FILE, "r");
  if (!file) {
    MyLog(ERROR, "Failed to open schedule file");
    schedule = defaultSchedule;
    return false;
    
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    MyLogF(ERROR, "Failed to parse schedule: %s", error.c_str());
    schedule = defaultSchedule;
    file.close();
    return false;
  }

  schedule.active = doc["active"];
  schedule.timezone = doc["timezone"].as<String>();
  schedule.startTime = doc["start_time"].as<String>();
  schedule.endTime = doc["end_time"].as<String>();

  schedule.daysCount = 0;
  for (JsonVariant day : doc["weekdays"].as<JsonArray>()) {
    if (schedule.daysCount < 7) {
      schedule.days[schedule.daysCount++] = day.as<String>();
    }
  }

  file.close();
  return true;
}


bool updateScheduleFromServer() {
    if (!connectToNetwork()) {
        MyLog(INFO, "Not connected - skipping schedule update");
        return false;
    }
    
    MyLog(INFO, "Checking for schedule updates...");
    HTTPClient http;
    
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/api/v1/esp/" + DEVICE_ID + "/schedule";
    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, "/api/v1/esp/" + DEVICE_ID + "/schedule");
    } else {
        http.begin(client, url);
    }
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        if (parseScheduleResponse(payload)) {
            MyLog(INFO, "Schedule updated from server");
            http.end();
            return true;
        }
    } else {
        MyLogF(ERROR, "Failed to get schedule: %d", httpCode);
    }
    
    http.end();
    return false;
}
