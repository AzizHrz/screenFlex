#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Hardware.h" 
#include "RTC_module.h"
#include "IR_Module.h"
#include <LittleFS.h>
#include <Arduino.h>
// #include "server_restAPI.py"


IRHandler irHandler(IR_RECEIVE_PIN, IR_SEND_PIN_, TV_STATUS_PIN);
uint8_t tmp = irHandler.getTVStatus(1000);
// Wi-Fi credentials
const char* ssid = "TP-Link_AP_70CA";
const char* password = "97800903";

// Server URL
const char* serverUrl = "http://127.0.0.1:5000";

// RTC Module parameters
RTC_Module rtc;
RtcDateTime now ;

void setup() {
  Serial.begin(115200);

   irHandler.begin();
   irHandler.init_littleFS();
   bool receiveStatus = irHandler.receiveIR(true);  

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  // RTC Module initialization
    rtc.RTC_Module_init();
    rtc.checkOperatingPeriod();
    rtc.set_beginTime(beginTime);
    rtc.set_endTime(endTime);

  // Make an HTTP GET request
  HTTPClient http;
  http.begin(serverUrl); // Connect to the server
  int httpCode = http.GET();

  // Check the HTTP response
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("JSON Response:");
    Serial.println(payload);

    // Parse the JSON response
    StaticJsonDocument<512> doc; // Adjust the size based on JSON structure
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      return;
    }

    // Extract data from JSON
    const char* beginTime = doc["schedule"]["begin"];
    const char* endTime = doc["schedule"]["end"];
    const char* tvState = doc["TV_State"];
    

    // Print extracted values
    Serial.print("Schedule Begin: ");
    Serial.println(beginTime);
    Serial.print("Schedule End: ");
    Serial.println(endTime);
    Serial.print("TV State: ");
    Serial.println(tvState);
   

    // Example: Use extracted data
    if (String(tvState) == "on") {
    Serial.println("The TV is currently ON.");
    }

  } else {
    Serial.print("HTTP GET request failed with code: ");
    Serial.println(httpCode);
  }

  http.end(); // Close the HTTP connection
}

void loop() {
   if (String(tvState) == "on") 
   {
        //cheak tv actual state 
        if (!tmp) {

            irHandler.sendIR();
        }else {
            pritnf("tv is already on");
                }

        
    } else if (String(tvState) == "off") 
    {
    //cheak tv actual state 
    if (tmp) {
        irHandler.sendIR();
    }else {
        pritnf("tv is already off");
            }
        
    
    }else {
        now = rtc.RTC_getDateTime();
        if (rtc.isWithinOperatingPeriod(now)) {
            //cheak tv actual state 
            if (!tmp) {
            
            //turn on tv
            irHandler.sendIR();
            
            }else {
            pritnf("tv is already on");
                    }

        }
        } else {
            //cheak tv actual state 
            if (tmp) {
                //turn off tv
                irHandler.sendIR();
            }else {
                pritnf("tv is already off");
                    }
                 }
    }

