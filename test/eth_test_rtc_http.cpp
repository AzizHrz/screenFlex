#include <Ethernet.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Hardware.h"
#include "RTC_module.h"
#include "IR_Module.h"
#include <LittleFS.h>
#include <Arduino.h>
#include "server_restAPI.py"

IRHandler irHandler(IR_RECEIVE_PIN, IR_SEND_PIN_, TV_STATUS_PIN);
uint8_t tmp = irHandler.getTVStatus(1000);

// Ethernet settings (DHCP will automatically configure these)
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address for the Ethernet shield

// Server URL
const char* serverUrl = "http://127.0.0.1:5000";

// RTC Module parameters
RTC_Module rtc;
RtcDateTime now;

// Global variables to store TV state and schedule
const char* tvState = "off";
const char* beginTime = "00:00";
const char* endTime = "00:00";

void setup() {
  Serial.begin(115200);

  irHandler.begin();
  irHandler.init_littleFS();
  bool receiveStatus = irHandler.receiveIR(true);

  // Initialize Ethernet with DHCP
  Serial.println("Initializing Ethernet with DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP.");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    while (true); // Halt if Ethernet connection fails
  }

  // Print network configuration obtained from DHCP
  Serial.println("Ethernet initialized with DHCP:");
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("Subnet mask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("DNS server: ");
  Serial.println(Ethernet.dnsServerIP());

  // RTC Module initialization
  rtc.RTC_Module_init();
  rtc.checkOperatingPeriod();
  rtc.set_beginTime(beginTime);
  rtc.set_endTime(endTime);

  // Fetch initial schedule and TV state from the server
  fetchServerData();
}

void loop() {
  // Fetch the latest data from the server periodically
  static unsigned long lastFetchTime = 0;
  if (millis() - lastFetchTime > 60000) { // Fetch every 60 seconds
    fetchServerData();
    lastFetchTime = millis();
  }

  // Get the current time
  now = rtc.RTC_getDateTime();

  // Check if the TV should be on or off based on the schedule or direct command
  bool shouldTVBeOn = false;

  if (String(tvState) == "on") {
    shouldTVBeOn = true;
  } else if (String(tvState) == "off") {
    shouldTVBeOn = false;
  } else {
    // If no direct command, follow the schedule
    shouldTVBeOn = rtc.isWithinOperatingPeriod(now);
  }

  // Check the actual TV state and turn it on/off if necessary
  uint8_t currentTVState = irHandler.getTVStatus(1000);

  if (shouldTVBeOn && !currentTVState) {
    // TV should be on but it's off
    irHandler.sendIR();
    Serial.println("Turning TV ON");
  } else if (!shouldTVBeOn && currentTVState) {
    // TV should be off but it's on
    irHandler.sendIR();
    Serial.println("Turning TV OFF");
  }

  delay(1000); // Small delay to avoid excessive loop iterations
}

void fetchServerData() {
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
    beginTime = doc["schedule"]["begin"];
    endTime = doc["schedule"]["end"];
    tvState = doc["TV_State"];

    // Update the RTC schedule
    rtc.set_beginTime(beginTime);
    rtc.set_endTime(endTime);

    // Print extracted values
    Serial.print("Schedule Begin: ");
    Serial.println(beginTime);
    Serial.print("Schedule End: ");
    Serial.println(endTime);
    Serial.print("TV State: ");
    Serial.println(tvState);

  } else {
    Serial.print("HTTP GET request failed with code: ");
    Serial.println(httpCode);
  }

  http.end(); // Close the HTTP connection
}