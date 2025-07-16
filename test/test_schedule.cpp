

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

    
    Network.configureOTA(false);
    // Network.setServerIp(SERVER_IP) ;
    // Network.setSercerPort(SERVER_PORT) ;

    Network.configureWiFi(ssid, password);
//    Network.InitModule();
    Network.InitWifiModule();
    Network.setServerIp(mip);
    Network.setSercerPort(5000);

    // Network.
    delay(3000);
    MyLog(INFO, "DONE");
    Serial.println(Network.GetFirmwareVersion());

    MyLog(INFO, "Getting schedul ");
    String input = Network.GetSchedule_HTTP(); 
    ScheduleData currentSchedule; 
    // parseScheduleJson(input, currentSchedule);
    handleScheduleResponse(input);
    


    // Serial.println(100/0);
}

void loop()
{
   Serial.println("Looping...");
   rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
   delay(1000);
    
}