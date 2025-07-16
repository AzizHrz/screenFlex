

#include <Arduino.h>
#include "network_stack.h"
#include "Mylog.h"
#include <ArduinoJson.h> 
#include "schedule.h" 

#include "RGBModule.h"
#include "Hardware.h"

#pragma message("Compiling test_schedule.cpp")

bool parseScheduleJson(const String& jsonResponse, ScheduleData& schedule) {
  DynamicJsonDocument doc(2048); // Adjust size based on your JSON complexity
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  // Extract basic fields
  schedule.active = doc["data"]["active"] | false;
  schedule.timezone = doc["data"]["timezone"].as<String>();

  // Initialize all days as disabled
  for (int i = 0; i < 7; i++) {
    schedule.dayEnabled[i] = false;
    schedule.days[i].periods.clear();
  }

  // Process each day in the weekdays array
  JsonArray weekdays = doc["data"]["weekdays"];
  for (JsonObject dayEntry : weekdays) {
    int dayIndex = dayEntry["day"]; // 0=Sunday to 6=Saturday
    if (dayIndex >= 0 && dayIndex < 7) {
      schedule.dayEnabled[dayIndex] = true;

      // Process each time period for this day
      JsonArray periods = dayEntry["periods"];
      for (JsonObject period : periods) {
        TimePeriod p;
        p.startTime = period["start_time"].as<String>();
        p.endTime = period["end_time"].as<String>();
        schedule.days[dayIndex].periods.push_back(p);
      }
    }
  }

  return true;
}

void handleScheduleResponse(const String& jsonResponse) {
  ScheduleData currentSchedule;
  
  if (parseScheduleJson(jsonResponse, currentSchedule)) {
    Serial.println("Successfully parsed schedule:");
    Serial.print("Active: "); Serial.println(currentSchedule.active);
    Serial.print("Timezone: "); Serial.println(currentSchedule.timezone);
    
    for (int day = 0; day < 7; day++) {
      if (currentSchedule.dayEnabled[day]) {
        Serial.printf("Day %d has %d periods:\n", day, currentSchedule.days[day].periods.size());
        for (const TimePeriod& period : currentSchedule.days[day].periods) {
          Serial.printf("  %s to %s\n", period.startTime.c_str(), period.endTime.c_str());
        }
      }
    }
  }
}

NetworkStack Network;
// myserv: (192,168,1,14) , 5000
// bilel: (203,161,60,31) , 3500
//const char *password = "i6q4k3b21r"; //"97800903";
//const char *ssid = "TT_23C0";        //"TP-Link_AP_70CA";

// ssids 
// const char* ssid = "Guest-9BEE" ; // "Aziz_s_iPhone";
// const char* password = "orange123"; //"just100$";


const char* ssid =  "Aziz_s_iPhone";
const char* password = "just100$";


RGBModule rgb(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);


bool is_newer_version(const String& current, const String& available) {
    int c_major, c_minor, c_patch;
    int a_major, a_minor, a_patch;

    sscanf(current.c_str(), "%d.%d.%d", &c_major, &c_minor, &c_patch);
    sscanf(available.c_str(), "%d.%d.%d", &a_major, &a_minor, &a_patch);

    if (a_major > c_major) return true;
    if (a_major == c_major && a_minor > c_minor) return true;
    if (a_major == c_major && a_minor == c_minor && a_patch > c_patch) return true;

    return false;
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
    saveCurrentVersion(FIRMWARE_VERSION); // FIRMWARE_VERSION should be defined
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


void setup()
{
    // init RGB
   rgb.RGBInit();
   rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);
    Serial.begin(115200);

    //192.168.1.173
    // IPAddress mip(192, 168, 1, 173);
// 172.20.10.2:5000
    IPAddress mip(172, 20, 10,3);

    Network.setServerIp(mip);
    Network.setSercerPort(5000);

    
    Network.configureOTA(true);
    // Network.setServerIp(SERVER_IP) ;
    // Network.setSercerPort(SERVER_PORT) ;

    Network.configureWiFi(ssid, password);
//    Network.InitModule();
    Network.InitWifiModule();
   // Network.setServerIp(mip);
    //Network.setSercerPort(5000);

    

    MyLog(INFO, "Getting schedul ");
    String input = Network.GetSchedule_HTTP(); 
    ScheduleData currentSchedule; 
    // parseScheduleJson(input, currentSchedule);
    handleScheduleResponse(input);
    
        // Network.
    delay(3000);
    MyLog(INFO, "DONE");
    String newVersion = Network.GetFirmwareVersion();
    MyLogF(INFO, "Server version: %s", newVersion.c_str());

    Serial.println(newVersion);

    String currentVersion = loadCurrentVersion();
    MyLogF(INFO, "Current version: %s", currentVersion.c_str());
    if (is_newer_version(currentVersion, newVersion)) {

    MyLog(INFO, "Performing OTA ");
    bool st = Network.OTA();
    if (st)
    {
        MyLog(INFO, "RESTATRING OTA ");
        ESP.restart();
    }
    else
    {
        MyLog(INFO, " OTA Failed miserably ");
    }

    }
    // Serial.println(100/0);
}

void loop()
{
   Serial.println("Looping...");
   rgb.setBlinkingStatus(RED_COLOR_STATUS);
   delay(1000);
    
}