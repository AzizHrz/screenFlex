// /*
//  * Software Verision 3.2.1 , HARZALLAH Aziz
//  *31/07/2025
//  * @screen_flex
//  * ScreenFlex application using MQTT, with OTA check during setup()
//  * if a newer firmware version exists on the Flask server

//  */

#include "conf.h"
#include "soft_version.h"
#include "network_stack.h"
#include "Hardware.h"
#include "RGBModule.h"
#include "ButtonModule.h"
// #include "IR_Module.h"
#include "IRModule.h"
// #include "RTC_module.h"
#include "RTCModule.h"
#include "Mylog.h"
#include <Arduino.h>

#include <HTTPUpdate.h> // Include the ESP32 HTTP Update library

// MQTT updates =======================================================
#include <PubSubClient.h>
#include <ArduinoJson.h>
const char *mqtt_server = "broker.hivemq.com";
const char *deviceId = "esp32-01"; // DEVICE ID

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topics with device ID
// String orderTopic = "screenflex/" + String(deviceId) + "/order";
// String stateTopic = "screenflex/" + String(deviceId) + "/state";
String orderTopic;
String stateTopic;
String firmwareTopic;

//========================================================

// ESP32-WEB-SERVER ========================================================
#include "ESP32_AP_STA.h"

//========================================================================

// IRHandler irHandler(IR_RECEIVE_PIN, IR_SEND_PIN_, TV_STATUS_PIN);
IRHandler irHandler(IR_RECEIVE_PIN, 32, TV_STATUS_PIN);
ButtonModule btn(PIN_BTN);
RGBModule rgb(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);
NetworkStack network;
RTC_Module rtc;
// volatile uint8_t Blinking_status = BLUE_COLOR_STATUS; // 0: Red, 1: Green, 2: Blue
volatile bool Wake_PC_STATE = false;
volatile bool Wake_PC_STATE_finished = false;
// pin defs

unsigned long btime = 0;

static unsigned long lastStateVerify = 0;
static unsigned long lastTimeVerify = 0;
static unsigned long lastOffTV = 0;
static unsigned long lastPing = 0;

RtcDateTime nowTime;

// ethnet configuration
bool useEthernet = false;
// WiFi configuration
// const char* ssid = "Aziz_s_iPhone";
// const char* passwd = "just100$";
// WiFi configuration
const char *ssid = "TP-Link_AP_70CA";
const char *passwd = "97800903"; // "i6q4k3b21r"; // "just100$";
// NetworkStack Network;

struct WiFiConfig
{
  String ssid;
  String password;
};

//==============================================================

EthernetClient ethClient;
// WiFiClient wifi;

IPAddress serverIp(SERVER_IP); // Change to your Flask server IP 172.20.10.2 //18 ,235, 73, 121
const int serverPort = SERVER_PORT;

// TV State Enumeration - Add this before PersistentData struct
/*
enum TVState {
  TV_OFF,          // 0 Confirmed OFF state
  TV_ON,           // 1  ON state
  TV_UNKNOWN,      // 2 Confirmed Initial/unknown state
  TV_TRANSITIONING // 3 Currently changing state
};
*/
#include "TVStateUtils.h"
// Global State Variables
TVState currentTVState = TV_UNKNOWN;
TVState desiredTVState = TV_UNKNOWN;
// TVState desiredTVState = TV_ON; // just for testing
TVState previousTVState = TV_UNKNOWN;

bool scheduleModeEnabled; // true = use schedule, false = use server commands
bool currentMode;         // true = use schedule, false = use server commands

// Schedule Data Structures
struct TimePeriod
{
  String startTime;
  String endTime;
};

struct ScheduleDay
{
  std::vector<TimePeriod> periods; // Multiple time periods for one day
};

struct ScheduleData
{
  // bool active;
  String timezone;
  bool dayEnabled[7];  // Sunday=0, ..., Saturday=6
  ScheduleDay days[7]; // One ScheduleDay per weekday
};

ScheduleData currentSchedule;

// Then declare your PersistentData struct
struct PersistentData
{
  bool scheduleModeEnabled; // true = use schedule, false = use server commands
  ScheduleData schedule;
  TVState lastCommandedState; // Add this to track last server command
  unsigned long lastUpdate;   // Timestamp of last update
};

// Add this global variable
PersistentData persistentData;

//
#define MODE_FILE "/mode.json"
#define SCHEDULE_FILE "/schedule.json"
#define WIFI_CONFIG_FILE "/wifi_config.json"
// #define IR_DATA_FILE "/ir_data.json"
// #define VERSION_FILE "/version.json"

String DEVICE_ID = "SOLUTIONGENERIQUE1";

bool saveCurrentVersion(const String &);
String loadCurrentVersion();
bool is_newer_version(const String &, const String &);

// Function to convert JSON variant to ScheduleData struct
void convertFromJson(JsonVariantConst src, ScheduleData &dst)
{
  dst.timezone = src["timezone"] | dst.timezone;
  MyLogF(DEBUG, "Timezone from JSON: %s", dst.timezone.c_str());

  // Initialize all days
  for (int i = 0; i < 7; i++)
  {
    dst.dayEnabled[i] = false;
    dst.days[i].periods.clear();
  }

  if (!src.containsKey("weekdays"))
  {
    MyLog(ERROR, "Missing 'weekdays' key in JSON");
    return;
  }

  JsonArrayConst weekdays = src["weekdays"];
  MyLogF(DEBUG, "'weekdays' array size: %d", weekdays.size());

  int dayIndex = 0;
  for (JsonObjectConst dayEntry : weekdays)
  {
    if (dayIndex >= 7)
    {
      MyLog(ERROR, "Too many entries in 'weekdays' array (max is 7)");
      break;
    }

    bool enabled = dayEntry["enabled"] | false;
    dst.dayEnabled[dayIndex] = enabled;
    MyLogF(DEBUG, "Day %d enabled: %s", dayIndex, enabled ? "true" : "false");

    if (!dayEntry.containsKey("slots"))
    {
      MyLogF(ERROR, "Day %d has no 'slots' key", dayIndex);
      dayIndex++;
      continue;
    }

    JsonArrayConst slots = dayEntry["slots"];
    MyLogF(DEBUG, "Day %d has %d slots", dayIndex, slots.size());

    for (JsonObjectConst slot : slots)
    {
      TimePeriod p;
      p.startTime = slot["start_time"] | "";
      p.endTime = slot["end_time"] | "";

      if (p.startTime == "" || p.endTime == "")
      {
        MyLogF(ERROR, "Day %d has slot with missing time fields", dayIndex);
        continue;
      }

      MyLogF(DEBUG, "  Slot: %s -> %s", p.startTime.c_str(), p.endTime.c_str());

      dst.days[dayIndex].periods.push_back(p);
    }

    MyLogF(DEBUG, "Total periods saved for day %d: %d", dayIndex, dst.days[dayIndex].periods.size());

    dayIndex++;
  }
}

bool saveMode(const bool &scheduleModeEnabled)
{
  if (!LittleFS.begin())
  {
    MyLog(ERROR, "Failed to mount LittleFS");
    return false;
  }
  File file = LittleFS.open(MODE_FILE, "w");
  if (!file)
  {
    MyLog(ERROR, "Failed to open mode file for writing");
    return false;
  }
  DynamicJsonDocument doc(128);
  doc["use_schedule"] = scheduleModeEnabled;
  size_t bytesWritten = serializeJson(doc, file);
  file.close();
  if (bytesWritten == 0)
  {
    MyLog(ERROR, "Failed to write mode");
    return false;
  }
  MyLogF(INFO, "Mode saved (%d bytes)", bytesWritten);
  return true;
}

bool loadMode(bool &scheduleModeEnabled)
{ // Load the mode from LittleFS
  if (!LittleFS.exists(MODE_FILE))
  {
    MyLog(INFO, "No mode file, using default (schedule)");
    scheduleModeEnabled = true; // Default to schedule
    return false;
  }
  File file = LittleFS.open(MODE_FILE, "r");
  if (!file)
  {
    MyLog(ERROR, "Failed to open mode file");
    scheduleModeEnabled = true; // Default to schedule
    return false;
  }
  DynamicJsonDocument doc(128);
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    MyLogF(ERROR, "Failed to parse mode: %s", error.c_str());
    scheduleModeEnabled = true; // Default to schedule
    file.close();
    return false;
  }
  scheduleModeEnabled = doc["use_schedule"];
  file.close();
  return true;
}

bool saveSchedule(const ScheduleData &schedule)
{
  if (!LittleFS.begin())
  {
    MyLog(ERROR, "Failed to mount LittleFS");
    return false;
  }

  File file = LittleFS.open(SCHEDULE_FILE, "w");
  if (!file)
  {
    MyLog(ERROR, "Failed to open schedule file for writing");
    return false;
  }

  DynamicJsonDocument doc(2048); // Adjust size as needed
  doc["timezone"] = schedule.timezone;

  JsonArray weekdaysArray = doc.createNestedArray("weekdays");
  for (int day = 0; day < 7; ++day)
  {
    JsonObject dayObj = weekdaysArray.createNestedObject();
    dayObj["enabled"] = schedule.dayEnabled[day];

    JsonArray slotsArray = dayObj.createNestedArray("slots");
    for (const TimePeriod &period : schedule.days[day].periods)
    {
      JsonObject slot = slotsArray.createNestedObject();
      slot["start_time"] = period.startTime;
      slot["end_time"] = period.endTime;
    }
  }

  size_t bytesWritten = serializeJson(doc, file);
  file.close();

  if (bytesWritten == 0)
  {
    MyLog(ERROR, "Failed to write schedule");
    return false;
  }

  MyLogF(INFO, "Schedule saved (%d bytes)", bytesWritten);
  return true;
}

bool saveWiFiConfig(const WiFiConfig &config)
{
  if (!LittleFS.begin())
  {
    MyLog(ERROR, "Failed to mount LittleFS");
    return false;
  }

  File file = LittleFS.open(WIFI_CONFIG_FILE, "w");
  if (!file)
  {
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

bool loadWiFiConfig(WiFiConfig &config)
{
  if (!LittleFS.begin())
  {
    MyLog(ERROR, "Failed to mount LittleFS");
    return false;
  }

  if (!LittleFS.exists(WIFI_CONFIG_FILE))
  {
    MyLog(INFO, "No WiFi config file found");
    return false;
  }

  File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
  if (!file)
  {
    MyLog(ERROR, "Failed to open WiFi config file for reading");
    return false;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    MyLogF(ERROR, "Failed to parse WiFi config: %s", error.c_str());
    return false;
  }


  config.ssid = doc["ssid"].as<String>();
  config.password = doc["password"].as<String>();
  return true;
}

void checkAndUpdateFirmware()
{

  String newVersion = network.GetFirmwareVersion();
  MyLogF(INFO, "Server version: %s", newVersion.c_str());

  String currentVersion = loadCurrentVersion();
  MyLogF(INFO, "Current version: %s", currentVersion.c_str());

  if (is_newer_version(currentVersion, newVersion))
  {
    MyLogF(INFO, "Detected a new version from sevrer %s with version %s",
           network.GetServerIP().toString(), newVersion);

    bool st = network.OTA();
    if (st)
    {
      MyLog(INFO, "RESTATRING ESP, AFTER OTA SUCCESS");
      saveCurrentVersion(newVersion); // ✅ Save new version after success
      ESP.restart();
    }
    else
    {
      MyLog(INFO, " OTA Failed miserably ");
    }
  }
  else
  {
    MyLogF(INFO, "Noooo new version from sevrer %s with version %s",
           network.GetServerIP().toString(), newVersion);
    MyLog(INFO, "Ignoring OTA.");
  }
}

bool connectToNetwork()
{
  // network.InitEthernetModule(W5500_CS);
  // by aziz

  if (!network.InitEthernetModule(W5500_CS))
  {
    // we chech 3 times if the ethernet module is initialized
    MyLog(INFO, "Failed to initialize Ethernet module");
    MyLog(INFO, "Falling back to WiFi");
    // network.configureWiFi(ssid, password);
    IPAddress ip SERVER_IP;
    network.setServerIp(ip);
    network.setSercerPort(SERVER_PORT);
    WiFiConfig loadedwifiConfig;
    if (loadWiFiConfig(loadedwifiConfig))
    {
      MyLogF(INFO, "Loaded WiFi config: SSID=%s", loadedwifiConfig.ssid.c_str());
      network.configureWiFi(loadedwifiConfig.ssid.c_str(), loadedwifiConfig.password.c_str());
    }
    else
    {
      MyLogF(INFO, "Using hard code it WiFi config: SSID=%s", ssid);       
    network.configureWiFi(ssid, passwd);
    }
    // network.InitWifiModule();

    // Initialize WiFi as both Access Point and Station==================
    initWiFiAPSTA(); // Setup WiFi & web server
    while (WiFi.status() != WL_CONNECTED)
    {
      handleWiFiServer(); // Handle client input from /connect
      delay(10);          // Small delay to prevent watchdog reset
    }

    // Connected successfully
    Serial.println("ESP32 connected as STATION");
    WiFi.softAPdisconnect(true); // Turn off AP
    Serial.println("Access Point disabled");

    // Optional: show IP or do other setup tasks here
    Serial.println("IP Address: " + WiFi.localIP().toString());
    //================================================================

    network.configureOTA(true);
    // new for screenFlex-esp-v3-2.cpp
    MyLog(INFO, "Checking for OTA updates...delay 3000ms");
    delay(3000);
    checkAndUpdateFirmware();
    useEthernet = false;
    return true;
  }
  else
  {
    MyLog(INFO, "Ethernet module initialized successfully");
    useEthernet = true;
    return true;
  }
}

bool saveCurrentVersion(const String &version)
{
  File file = LittleFS.open("/version.json", "w");
  if (!file)
  {
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

// to do : error handling
String loadCurrentVersion()
{
  if (!LittleFS.exists("/version.json"))
  {
    MyLog(INFO, "No version file found, creating one with default version");
    saveCurrentVersion(FIRMWARE_VERSION); // FIRMWARE_VERSION should be defined
    return FIRMWARE_VERSION;
  }

  File file = LittleFS.open("/version.json", "r");
  if (!file)
  {
    MyLog(ERROR, "Failed to open version file");
    return FIRMWARE_VERSION;
  }

  DynamicJsonDocument doc(128);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    MyLogF(ERROR, "Failed to parse version: %s", error.c_str());
    return FIRMWARE_VERSION;
  }

  return doc["version"].as<String>();
}

bool is_newer_version(const String &current, const String &available)
{
  int c_major, c_minor, c_patch;
  int a_major, a_minor, a_patch;

  sscanf(current.c_str(), "%d.%d.%d", &c_major, &c_minor, &c_patch);
  sscanf(available.c_str(), "%d.%d.%d", &a_major, &a_minor, &a_patch);
  // Compare major, minor, and patch versions
  if (a_major > c_major)
    return true;
  else if (a_major == c_major && a_minor > c_minor)
    return true;
  else if (a_major == c_major && a_minor == c_minor && a_patch > c_patch)
    return true;

  return false;
}

bool performOTAUpdate(String firmwareUrl)
{
  MyLog(INFO, "Starting OTA update...");
  rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);

  WiFiClient espClient;

  // Configure update callbacks
  HTTPUpdateResult ret = httpUpdate.update(espClient, firmwareUrl);

  bool success = false;

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    MyLogF(ERROR, "OTA failed: %s", httpUpdate.getLastErrorString().c_str());
    success = false;
    break;
  case HTTP_UPDATE_NO_UPDATES:
    MyLog(INFO, "No updates available");
    success = true; // Not a failure, but no update was applied
    break;
  case HTTP_UPDATE_OK:
    MyLog(INFO, "OTA update completed");
    success = true;
    break;
  }

  rgb.setBlinkingStatus(GREEN_COLOR_STATUS);

  return success;
}

// by aziz
// #define RTC_DS1307 // Uncomment this line if using DS1307 RTC
// String getCurrentDay() {
int getCurrentDay()
{

  RtcDateTime rtcTime = rtc.RTC_getDateTime();

  // For DS1307, we need to read day of week directly from register
  uint8_t dayOfWeek;

#ifdef RTC_DS1307
  Wire.beginTransmission(0x68);
  Wire.write(0x03); // Day of week register
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  if (Wire.available())
  {
    dayOfWeek = Wire.read();
  }
  else
  {
    dayOfWeek = 0;
  }
  // DS1307 uses 1=Sunday through 7=Saturday
  dayOfWeek = (dayOfWeek - 1) % 7;
#else
  // For other RTC chips that include day in DateTime
  dayOfWeek = rtcTime.DayOfWeek();
#endif

  // Validate day
  if (dayOfWeek > 6)
  {
    // Calculate from date if invalid
    int d = rtcTime.Day();
    int m = rtcTime.Month();
    int y = rtcTime.Year();

    if (m < 3)
    {
      m += 12;
      y--;
    }

    dayOfWeek = (d + (13 * (m + 1) / 5) + y + (y / 4) - (y / 100) + (y / 400)) % 7;
  }

  // const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  // return days[dayOfWeek % 7];
  return (dayOfWeek % 7);
}

bool checkSchedule(const ScheduleData &schedule)
{
  int currentWeekDay = getCurrentDay(); // 0 = Sunday, 6 = Saturday
  MyLogF(DEBUG, "currentWeekDay from RTC: %d", currentWeekDay);

  if (!schedule.dayEnabled[currentWeekDay])
  {
    MyLogF(DEBUG, "Day %d is disabled in schedule", currentWeekDay);
    return false;
  }

  RtcDateTime now = rtc.RTC_getDateTime();
  int currentMins = now.Hour() * 60 + now.Minute();
  MyLogF(DEBUG, "currentMins from RTC: %d", currentMins);

  const std::vector<TimePeriod> &periods = schedule.days[currentWeekDay].periods;
  MyLogF(DEBUG, "Number of periods for day %d: %d", currentWeekDay, periods.size());

  for (const TimePeriod &period : periods)
  {
    MyLogF(DEBUG, "Period: start = %s | end = %s", period.startTime.c_str(), period.endTime.c_str());

    int startH, startM, endH, endM;
    if (sscanf(period.startTime.c_str(), "%d:%d", &startH, &startM) != 2 ||
        sscanf(period.endTime.c_str(), "%d:%d", &endH, &endM) != 2)
    {
      MyLog(ERROR, "Invalid time format in schedule");
      continue;
    }

    int startMins = startH * 60 + startM;
    int endMins = endH * 60 + endM;

    MyLogF(DEBUG, "Parsed startMins: %d | endMins: %d", startMins, endMins);

    // Handle overnight windows (e.g., 22:00 – 06:00)
    if (endMins < startMins)
    {
      if (currentMins >= startMins || currentMins <= endMins)
      {
        MyLog(DEBUG, "Current time is within overnight period");
        return true;
      }
    }
    else
    {
      if (currentMins >= startMins && currentMins <= endMins)
      {
        MyLog(DEBUG, "Current time is within normal period");
        return true;
      }
    }
  }

  MyLog(DEBUG, "No active time period matched; schedule returns false");
  return false;
}

TVState updateDesiredTVState(const char *mode)
{
  if (strcmp(mode, "on") == 0)
  {
    MyLog(INFO, "TV ON command received");
    return TV_ON;
  }
  else if (strcmp(mode, "off") == 0)
  {
    MyLog(INFO, "TV Off command received");
    return TV_OFF;
  }
  else if (strcmp(mode, "schedule") == 0)
  {
    MyLog(INFO, "TV Schedule command received");
    return checkSchedule(currentSchedule) ? TV_ON : TV_OFF;
  }
  else
  {
    MyLogF(ERROR, "Unknown mode: %s", mode);
    return TV_UNKNOWN; // Handle unknown mode
  }
}

uint8_t checkTVStatus()
{
  static uint8_t lastStableState = 0;
  static uint8_t stableCount = 0;
  static unsigned long lastReadTime = 0;
  const uint8_t requiredStableReadings = 5;
  const unsigned long READ_INTERVAL = 150; // ms
  static uint8_t lastReadState = 0;

  // Check if it's time for a new reading
  if (millis() - lastReadTime >= READ_INTERVAL)
  {
    lastReadTime = millis();

    uint8_t currentState = digitalRead(TV_STATUS_PIN);

    if (currentState == lastReadState)
    {
      stableCount++;
    }
    else
    {
      stableCount = 0;
      lastReadState = currentState;
    }

    if (stableCount >= requiredStableReadings)
    {
      lastStableState = lastReadState;
    }
  }

  // Return known stable value if confirmed, or 2 if not yet stable
  return (stableCount >= requiredStableReadings) ? lastReadState : 2; // 0 =  UNKNOWN, 2 = ON
}

void manageTVState()
{
  // #if ENABLE_IR
  static unsigned long lastActionTime = 0;
  const unsigned long ACTION_COOLDOWN = 20000;

  // Skip if still in cooldown
  if (millis() - lastActionTime < ACTION_COOLDOWN)
    return;

  // Read current hardware state
  uint8_t hw = checkTVStatus();
  TVState actualState = (hw == 1) ? TV_OFF : TV_ON;
  // tvState = (actualState == TV_ON) ? "ON" : "OFF";

  // Keep global currentTVState updated
  if (currentTVState != TV_TRANSITIONING)
  {
    currentTVState = actualState;
  }

  // No change needed
  if (currentTVState == desiredTVState)
    return;

  MyLogF(INFO, "TV State Mismatch → Current: %s, Desired: %s, Mode: %s",
         currentTVState == TV_ON ? "ON" : "OFF",
         desiredTVState == TV_ON ? "ON" : "OFF",
         currentMode ? "schedule" : "manual");

  // Start state transition

  lastActionTime = millis();

  static unsigned long lastIRSendTime = 0;
  const unsigned long IR_SEND_COOLDOWN = 2000; // 2s

  // Only send if enough time passed
  if (millis() - lastIRSendTime >= IR_SEND_COOLDOWN)
  {
    currentTVState = TV_TRANSITIONING;
    lastIRSendTime = millis();

    irHandler.sendIR();
    MyLog(INFO, "IR command sent");
  }
  else
  {
    MyLog(INFO, "IR send blocked by 2s cooldown");
  }
  // Re-check TV state
  hw = checkTVStatus();
  TVState newState = (hw == 1) ? TV_OFF : TV_ON;

  if (newState == desiredTVState)
  {
    currentTVState = newState;
    MyLog(INFO, "TV state successfully changed");
    /*
    if (connectToNetwork()) {
        reportTVStateToServer();
    }*/
  }
  else
  {
    MyLog(ERROR, "TV state change failed. Reverting to last known");
    currentTVState = actualState;
  }
  // #endif
}

// by aziz ==================================================
//  Function to CHANGE TV state based on the current mode
void changeTVState()
{
  MyLog(DEBUG, "Sending IR command: current state incompatible with desired ...");
  while (!(irHandler.getTVStatus(1000) == desiredTVState == (TV_OFF || TV_ON) /* TV_ON we use pullup*/))
  {
    MyLog(DEBUG, "*");
    irHandler.sendIR();
    // tvOff == 1
  };
}

void refrechIRdata()
{
  rgb.setBlinkingStatus(WHITE_COLOR_STATUS);
  // clearr the json file that holds the IR data
  irHandler.clear_file(f);
  rgb.setBlinkingStatus(VIOLET_COLOR_STATUS);
  irHandler.receiveIR(true);
  rgb.setBlinkingStatus(BRIGHT_ORANGE_COLOR_STATUS);
}
// MQTT ============================================================

void setup_wifi()
{
  MyLog(INFO, "Connecting to WiFi...");
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  MyLog(INFO, "Connected to WiFi");
}
// Separate callback for firmwareTopic
void firmwareCallback(char *topic, byte *payload, unsigned int length)
{
  MyLog(INFO, " FIRMWARE TOPIC MESSAGE RECEIVED");

  // Convert payload into a proper String for logging
  String firmwareVersion;
  for (unsigned int i = 0; i < length; i++)
  {
    firmwareVersion += (char)payload[i];
  }

  MyLogF(INFO, " Firmware Version: %s", firmwareVersion.c_str());
  MyLog(INFO, " checkAndUpdateFirmware called");

  checkAndUpdateFirmware();
}

// Separate callback for orderTopic
void orderCallback(char *topic, byte *payload, unsigned int length)
{
  MyLogF(INFO, " ORDER TOPIC MESSAGE RECEIVED, length: %d", length);

  String jsonStr;
  for (unsigned int i = 0; i < length; i++)
  {
    jsonStr += (char)payload[i];
  }

  MyLogF(DEBUG, "Payload: %s", jsonStr.c_str());

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);

  if (error)
  {
    MyLogF(ERROR, "deserializeJson() failed: %s", error.c_str());
    return;
  }
  JsonObject data = doc["wifi"];

  //if (data.containsKey("wifi"))
  //{
    WiFiConfig newConfig;
    newConfig.ssid = data["wifi"]["ssid"].as<String>();
    newConfig.password = data["wifi"]["password"].as<String>();

    WiFiConfig currentConfig;
    if (loadWiFiConfig(currentConfig))
    {
      if (currentConfig.ssid != newConfig.ssid || currentConfig.password != newConfig.password)
      {
        if (saveWiFiConfig(newConfig))
        {
          MyLog(INFO, "Updated WiFi config from server");
          // Reconnect if we're using WiFi
          if (!useEthernet)
          {
            network.configureWiFi(newConfig.ssid.c_str(), newConfig.password.c_str());
            network.InitWifiModule();
          }
        }

      }
    }
    else
    {
      // No existing config, save the new one
      if (saveWiFiConfig(newConfig))
      {
        MyLog(INFO, "Saved new WiFi config from server");
      }
    }
  //}
  // Extract and log mode
  const char *mode = doc["mode"];
  if (!mode)
  {
    MyLog(ERROR, "Missing 'mode' in JSON");
    return;
  }

  MyLogF(DEBUG, "Mode: %s", mode);

  // If mode is "schedule", extract and parse the schedule
  if (strcmp(mode, "schedule") == 0)
  {
    JsonVariant scheduleJson = doc["schedule"];
    if (scheduleJson.isNull())
    {
      MyLog(ERROR, "Missing 'schedule' field");
      return;
    }

    currentSchedule = scheduleJson.as<ScheduleData>();
    saveSchedule(currentSchedule);
    MyLog(INFO, "Schedule saved with .as<ScheduleData>()");

    // Debug print days and slots
    for (int i = 0; i < 7; ++i)
    {
      bool enabled = currentSchedule.dayEnabled[i];
      MyLogF(DEBUG, "Day %d - Enabled: %s", i, enabled ? "true" : "false");

      for (const TimePeriod &slot : currentSchedule.days[i].periods)
      {
        MyLogF(DEBUG, "  Slot: %s -> %s", slot.startTime.c_str(), slot.endTime.c_str());
      }
    }
  }
  // Update desired state using mode
  desiredTVState = updateDesiredTVState(mode);
  MyLogF(DEBUG, "Desired TV state set to: %d", desiredTVState);
}

// Main callback router
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("📥 MQTT MESSAGE RECEIVED");
  Serial.print(" Topic: ");
  Serial.println(topic);
  Serial.print(" Payload size: ");
  Serial.print(length);
  Serial.println(" bytes");

  String topicStr = String(topic);

  if (topicStr == orderTopic)
  {
    Serial.println(" Routing to ORDER handler");
    orderCallback(topic, payload, length);
  }
  else if (topicStr == firmwareTopic)
  {
    Serial.println(" Routing to FIRMWARE handler");
    firmwareCallback(topic, payload, length);
  }
  else
  {
    Serial.print("Unknown topic: ");
    Serial.println(topicStr);
    Serial.print(" Message: ");
    for (int i = 0; i < length; i++)
      Serial.print((char)payload[i]);
    Serial.println("\n");
  }
}

void reconnect()
{
  while (!client.connected())
  {
    client.connect("esp32-client");
  }
  client.subscribe(orderTopic.c_str()); // Subscribe to new topic
}

//============================================================================
void setup()
{
  Serial.begin(115200);
  MyLog(INFO, "----------------------------------------");
  MyLogF(INFO, "Runing %s with version %s", PRODUCT_NAME, FIRMWARE_VERSION);
  MyLog(INFO, "----------------------------------------");

  // init storege and RGB LED
  irHandler.begin();
  irHandler.init_littleFS(); // TODO: add option to clear FS
  rgb.RGBInit();
  rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
  btn.InitButton();
  // init tasks
  rgb.runRGBTask();
  // delay(3000);
  /////////////////////////////////////////////////////////////
  rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);

  if (!rtc.begin())
  {
    // Serial.println("Failed to initialize RTC");
    MyLog(INFO, "Failed to initialize RTC");
    while (1)
      ;
  }

  // by aziz
  // Serial.println("============ Time Debug ============");
  MyLog(INFO, "============ Time Debug ============");
  MyLog(INFO, "RTC Current Time: ");
  nowTime = rtc.RTC_getDateTime();
  Serial.println(rtc.rtcDateTimeToString(nowTime));

  // initize internet Module
  MyLog(INFO, "Initializing Network Module ... + delay 300ms");
  // Network.configureWiFi(ssid, passwd);
  // Network.InitWifiModule();


  //Serial.print(" ESP32 Board MAC Address: ");
  // ***i get and use the mac addresse in pingTokenServer()
  /*uint8_t* mac = network.GetESP32Macaddr();
if (mac) {
Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
              mac[0], mac[1], mac[2],
              mac[3], mac[4], mac[5]);
}
*/
  /* String macStr;
    macStr = String(network.MacArrayToString(network.GetESP32Macaddr(), 18));
    Serial.println(macStr);
 */

  // connect to network and OTA
  static bool connected = connectToNetwork();

  delay(3000);

  rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
  String token = network.pingTokenServer(); ///////////////////////////////////////////////////////
  MyLog(INFO, "ping token server ...");
  String user = network.getUserFromServer();
  MyLogF(INFO, "User from server: %s", user.c_str());

  // mqtt setup =================================================
  // setup_wifi(); // same as InitWifiModule in connectToNetwork()

  // MQTT topics with device ID
  orderTopic = String(user) + "/" + String(token) + "/order";
  stateTopic = String(user) + "/" + String(token) + "/state";
  firmwareTopic = String(user) + "/" + String(token) + "/firmware";

  MyLogF(INFO, "Order Topic: %s", orderTopic.c_str());
  MyLogF(INFO, "State Topic: %s", stateTopic.c_str());
  MyLogF(INFO, "Firmware Topic: %s", firmwareTopic.c_str());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  MyLog(INFO, "Finished Initializing all the Modules  ...");
  MyLog(INFO, "Starting Now ...");
  // MyLog(DEBUG, "STARTING IR  ...");

  bool IRdataState = irHandler.IsIRDataavailable(f);

  /*
  //lust for test

  */
#define testserver
#ifdef testserver
  if (IRdataState)
  { // isDataAvailable(settings)) {
    rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);
    MyLog(DEBUG, "IR COMMAND FOUND  ...");
    // irHandler.dump_data_stored(f);
    MyIRData data;
    MyLog(DEBUG, "Reading data from json file ...");
    // irHandler.DumpMyIRDataJSON(f);
    irHandler.readMyIRDataJSON(f, data);
    // MyLog(DEBUG,"Printing data  ...");
    // irHandler.PrintMyIRData(data);
    irHandler.setRecievedData(data);
    rgb.setBlinkingStatus(BRIGHT_ORANGE_COLOR_STATUS);
    /*MyLog(DEBUG, "Sending IR signal if tv off ...");
    while (!(irHandler.getTVStatus(1000) == TV_ON /*we use pullup*/
    /*))
{
MyLog(DEBUG, ".");
irHandler.sendIR();
// tvOff == 1
} ;
*/
    // Blinking_status=GR;
  }
  else
  {
    MyLog(ERROR, "No data is avalaible LITTLEFS IS EMPTY, SCANNING NOW ....");

    // Perform the scanning operation
    rgb.setBlinkingStatus(WHITE_COLOR_STATUS);

    irHandler.receiveIR(true);
    rgb.setBlinkingStatus(BRIGHT_ORANGE_COLOR_STATUS);
      /*MyLog(DEBUG, "Sending IR signal if tv off ...");
    
       while (!(irHandler.getTVStatus(1000) == 1 ))
      {
         MyLog(DEBUG, ".");
     
        irHandler.sendIR();
         // tvOff == 1
      };
   */}
      MyLog(INFO, "Finished checking IR data");
      rgb.setBlinkingStatus(GREEN_COLOR_STATUS);

#endif
}
void loop()
{
  btn.HandleButton_5s();
  if (btn.isLongPressed_5s())
  {
    MyLog(DEBUG, "BUtton PRESSD for more than 5s ....");

    /*MyLog(DEBUG, "Sending IR signal if tv off ...");
    while (!(irHandler.getTVStatus(1000) == TV_ON ))
    {
      MyLog(DEBUG, ".");

      irHandler.sendIR();
      // tvOff == 1
    };
    MyLog(INFO, "Finished Sending IR data ");
    rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
 */
  }
  else
  {
#define testemeteurIR
#ifdef testemeteurIR

    static unsigned long lastReadTime = 0;
    const uint16_t READ_INTERVAL = 1000; // ms

    // MyLog(INFO, "MQTT Client Loop ...");
    if (!client.connected())
      reconnect();
    client.loop();

    if (millis() - lastReadTime >= READ_INTERVAL)
    {
      lastReadTime = millis();

      switch (irHandler.getTVStatus(1000))
      {
      case 0: // TV_OFF
        currentTVState = TV_OFF;
        MyLog(INFO, "TV is currently OFF");
        if (desiredTVState != currentTVState)
        {
          MyLog(INFO, "currentTVSTtae differs for desired state");
          changeTVState();
        }
        break;
      case 1: // TV_ON
        currentTVState = TV_ON;
        MyLog(INFO, "TV is currently ON");
        // MyLogF(INFO, "currentTVState : %s | desiredTVState %s", toString(currentTVState), toString(desiredTVState));
        if (desiredTVState != currentTVState)
        {
          MyLog(INFO, "currentTVSTtae differs for desired state");

          changeTVState();
        }
        break;
      case 2: // TV_TRANSITIONING
        currentTVState = TV_TRANSITIONING;
        MyLog(INFO, "TV is currently transitioning state");
        break;
      case 3: // TV_UNKNOWN
        currentTVState = TV_UNKNOWN;
        MyLog(INFO, "Current TV State is UNKNOWN");
        break;
      default:
        MyLog(ERROR, "Unknown TV state read from hardware");
        break;
      }
      MyLogF(INFO, "currentTVState : %s | desiredTVState %s", toString(currentTVState), toString(desiredTVState));
    }

    // // Send TV state every second
    // static unsigned long lastMsg = 0;
    // if (millis() - lastMsg > 1000) {
    //   lastMsg = millis();
    if (previousTVState != currentTVState)
    {

      StaticJsonDocument<128> doc;
      doc["millis"] = millis();
      doc["currentTVState"] = currentTVState;
      doc["desiredTVState"] = desiredTVState;

      char msg[128];
      size_t len = serializeJson(doc, msg);
      client.publish(stateTopic.c_str(), msg, len);
    }
    previousTVState = currentTVState;
#endif
  }
}