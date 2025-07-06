

/*
 * TV Controller Firmware with OTA Updates
 * Version: 1.3.6
 * Author: Ayoub Zaghouani
 */

/**/
#include "conf.h"
#include "soft_version.h"
#include "network_stack.h"
#include "Hardware.h"
#include "RGBModule.h"
#include "ButtonModule.h"
#include "IR_Module.h"
#include "Mylog.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h> // Include the ESP32 HTTP Update library
#include <ArduinoJson.h>
#include "RtcDateTime.h"
#include "RtcDS1307.h"
#include "RTC_module.h"
#include <LittleFS.h>
#include <time.h>
#include <TimeLib.h>
#include <Ethernet.h>
#include <SPI.h>

// Feature Configuration
#define ENABLE_IR 1  // Disabled IR module temporarily
#define ENABLE_WOL 0 // Wake on LAN feature temporarily disabled
#define ENABLE_OTA 1
#define SYNC_INTERVAL 300000    // Sync with server every 60 seconds
#define OTA_CHECK_INTERVAL 3600000  // Check for updates every hour

#define IR_DATA_FILE "/ir_data.json"
#define SCHEDULE_FILE "/schedule.json"
#define MODE_FILE "/mode.json"
#define VERSION_FILE "/version.json"
#define WIFI_CONFIG_FILE "/wifi_config.json"

const String CURRENT_FIRMWARE_VERSION = "1.0.0"; // ← Change this manually before build

struct WiFiConfig {
  String ssid;
  String password;
};


struct ModeData {
  bool useSchedule; // true = use schedule, false = use server commands
};
ModeData currentMode;

struct ServerCommand {
  bool state;
};
ServerCommand currentCommand;

struct ScheduleData {
  bool active;
  String days[7];
  int daysCount;
  String startTime;
  String endTime;
  String timezone;
};


ScheduleData currentSchedule;


// Network Configuration
const char* ssid = "OPPO Reno12 F";
const char* password = "h4qu37mv";

WiFiClient client;

// Ethernet Configuration
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Change if needed
const int W5500_CS_PIN = 33;
bool useEthernet = false;
EthernetClient ethClient;

IPAddress serverIp(18 ,235, 73, 121); // Change to your Flask server IP
const int serverPort = 5000;

// Global Objects
NetworkStack network;

#define IR_RECEIVE_PIN 16
#define TV_STATUS_PIN 39
#define IR_SEND_PIN 32

// uint8_t controlMode = MODE_SCHEDULE;

// Add near your other global variables
uint8_t currentTVStatus = 0;  
unsigned long lastStatusCheck = 0;
const uint16_t STATUS_CHECK_INTERVAL = 300; // Check every 300ms

String DEVICE_ID = "SOLUTIONGENERIQUE1";

#if ENABLE_IR
IRHandler irHandler(IR_RECEIVE_PIN, IR_SEND_PIN, TV_STATUS_PIN);
#endif

// TV State Enumeration - Add this before PersistentData struct
enum TVState {
  TV_UNKNOWN,      // Initial/unknown state
  TV_OFF,          // Confirmed OFF state
  TV_ON,           // Confirmed ON state
  TV_TRANSITIONING // Currently changing state
};

// Then declare your PersistentData struct
struct PersistentData {
    ModeData mode;
    ScheduleData schedule;
    TVState lastCommandedState; // Add this to track last server command
    unsigned long lastUpdate;   // Timestamp of last update
};

// Global State Variables
TVState currentTVState = TV_UNKNOWN;
TVState desiredTVState = TV_UNKNOWN;


// Add this global variable
PersistentData persistentData;

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
    
    // Corrected this line to use doc instead of data
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

// Enhanced load function
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

    // Load mode
    persistentData.mode.useSchedule = doc["mode"]["use_schedule"];
    
    // Load schedule
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
    
    // Load last command
    persistentData.lastCommandedState = (TVState)(int)doc["last_command"];
    persistentData.lastUpdate = doc["last_update"];
    
    file.close();
    
    // Update current variables
    currentMode = persistentData.mode;
    currentSchedule = persistentData.schedule;
    
    if (!currentMode.useSchedule) {
        desiredTVState = persistentData.lastCommandedState;
    }
    
    MyLog(INFO, "Persistent data loaded successfully");
    return true;
}

void convertFromJson(JsonVariantConst src, ScheduleData& dst) {
    dst.active = src["active"] | dst.active;
    dst.timezone = src["timezone"] | dst.timezone;
    dst.startTime = src["start_time"] | dst.startTime;
    dst.endTime = src["end_time"] | dst.endTime;

    dst.daysCount = 0;
    JsonArrayConst daysArray = src["weekdays"];
    for (JsonVariantConst day : daysArray) {
        if (dst.daysCount < 7) {
            dst.days[dst.daysCount++] = day.as<String>();
        }
    }
}


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

ButtonModule btn(PIN_BTN);
RGBModule rgb(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);
RTC_Module rtc;

uint8_t checkTVStatus() {
    static uint8_t lastStableState = 0;
    static uint8_t stableCount = 0;
    const uint8_t requiredStableReadings = 5;
    
    // Add small delay between readings
    delay(150);
    
    uint8_t currentState = digitalRead(TV_STATUS_PIN);
    
    if (currentState == lastStableState) {
        stableCount++;
    } else {
        stableCount = 0;
        lastStableState = currentState;
    }
    
    if (stableCount >= requiredStableReadings) {
        return lastStableState;
    }
    
    return 2; // 2 = unstable/unknown
}

bool initEthernet() {
    static unsigned long lastAttempt = 0;
    const unsigned long RETRY_INTERVAL = 60000; // 1 minute between retries
    
    if (millis() - lastAttempt < RETRY_INTERVAL && lastAttempt != 0) {
        return false;
    }
    
    lastAttempt = millis();
    
    pinMode(W5500_CS_PIN, OUTPUT);
    digitalWrite(W5500_CS_PIN, HIGH);
    SPI.begin();
    
    Ethernet.init(W5500_CS_PIN);
    MyLog(INFO, "Starting Ethernet connection...");
    
    if (Ethernet.begin(mac, 10000) == 0) { // 10 second timeout
        MyLog(ERROR, "Failed to configure Ethernet using DHCP");
        return false;
    }
    
    MyLogF(INFO, "Ethernet connected! IP: %s", Ethernet.localIP().toString().c_str());
    return true;
}

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

// OTA Update Components
#if ENABLE_OTA
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>

WebServer otaServer(80);

// Global Variables

bool receivedNewSchedule = false; 


bool connectToNetwork() {
    static WiFiConfig wifiConfig;
    static bool configLoaded = false;
    static unsigned long lastNetworkSwitch = 0;
    const unsigned long MIN_SWITCH_INTERVAL = 30000; // 30 seconds
    
    // Load WiFi config if not already loaded
    if (!configLoaded) {
        if (loadWiFiConfig(wifiConfig)) {
            MyLog(INFO, "Loaded WiFi config from storage");
        } else {
            // Fallback to compiled credentials
            wifiConfig.ssid = ssid;
            wifiConfig.password = password;
            MyLog(INFO, "Using compiled WiFi credentials");
        }
        configLoaded = true;
    }

    // Don't switch too frequently
    if (millis() - lastNetworkSwitch < MIN_SWITCH_INTERVAL && lastNetworkSwitch != 0) {
        return useEthernet ? Ethernet.linkStatus() == LinkON : WiFi.status() == WL_CONNECTED;
    }
    
    // Try Ethernet first if not already using it
    if (!useEthernet) {
        if (initEthernet()) {
            useEthernet = true;
            lastNetworkSwitch = millis();
            return true;
        }
    }
    
    // Check current connection status
    if (useEthernet) {
        if (Ethernet.linkStatus() == LinkOFF) {
            MyLog(INFO, "Ethernet disconnected, switching to WiFi");
            useEthernet = false;
            
            // Try saved WiFi first
            network.configureWiFi(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
            network.InitWifiModule();
            
            // Check if connection succeeded
            if (WiFi.status() != WL_CONNECTED) {
                // If saved WiFi fails, try hardcoded credentials
                MyLog(WARNING, "Saved WiFi failed, trying hardcoded credentials");
                network.configureWiFi(ssid, password);
                network.InitWifiModule();
            }
            
            lastNetworkSwitch = millis();
            return WiFi.status() == WL_CONNECTED;
        }
        return true;
    } else {
        if (WiFi.status() != WL_CONNECTED) {
            // Try saved WiFi first
            network.configureWiFi(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
            network.InitWifiModule();
            
            // Check if connection succeeded
            if (WiFi.status() != WL_CONNECTED) {
                // If saved WiFi fails, try hardcoded credentials
                MyLog(WARNING, "Saved WiFi failed, trying hardcoded credentials");
                network.configureWiFi(ssid, password);
                network.InitWifiModule();
            }
            lastNetworkSwitch = millis();
        }
        return WiFi.status() == WL_CONNECTED;
    }
}

void reportTVStateToServer() {
    if (!connectToNetwork()) {
        MyLog(INFO, "Not connected - skipping state report");
        return;
    }
    
    uint8_t currentHwState = checkTVStatus();
    String currentState = currentHwState ? "off" : "on";
    String path = "/api/v1/esp/" + DEVICE_ID + "/state";
    
    HTTPClient http;
    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, path);
    } else {
        http.begin(client, "http://" + serverIp.toString() + ":" + String(serverPort) + path);
    }
    
    http.addHeader("Content-Type", "application/json");
    String postData = "{\"actual_state\":\"" + currentState + "\"}";
    
    MyLogF(INFO, "Reporting TV state: %s", currentState.c_str());
    
    int httpCode = http.POST(postData);
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        MyLogF(INFO, "State report successful. Response: %s", payload.c_str());
        
        // Parse response to confirm server received the state
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error && doc["success"]) {
            MyLog(INFO, "Server acknowledged state report");
        }
    } else {
        MyLogF(ERROR, "State report failed: %d", httpCode);
        if(httpCode > 0) {
            String payload = http.getString();
            MyLogF(ERROR, "Error response: %s", payload.c_str());
        }
    }
    http.end();
}

void setupOTA() {
  // Initialize mDNS
  if (!useEthernet) {
        if (!MDNS.begin(DEVICE_ID.c_str())) {
            MyLog(ERROR, "Error setting up MDNS responder!");
            while (1) delay(1000);
        } else {
            MyLog(INFO, "mDNS responder started");
            MDNS.addService("http", "tcp", 80);
        }
    } else {
        MyLog(INFO, "Skipping mDNS setup on Ethernet");
    }
  
  // Configure OTA update server
  otaServer.on("/update", HTTP_GET, []() {
    otaServer.sendHeader("Connection", "close");
    otaServer.send(200, "text/html", 
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='update'>"
      "<input type='submit' value='Update'>"
      "</form>");
  });

  otaServer.on("/update", HTTP_POST, []() {
    otaServer.sendHeader("Connection", "close");
    otaServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = otaServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
      MyLog(INFO, "OTA Update Started");
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) {
        MyLog(ERROR, "OTA Update Begin Failed");
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        MyLog(ERROR, "OTA Write Failed");
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        MyLog(INFO, "OTA Update Success");
      } else {
        MyLog(ERROR, "OTA Update Failed");
      }
    }
  });
  
  otaServer.begin();
  MDNS.addService("http", "tcp", 80);
  MyLog(INFO, "OTA Update Server Ready");
}
#endif



// Schedule Variables
String tvState = "OFF";

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
    mode.useSchedule = true; // Default to schedule
    return false;
  }
  File file = LittleFS.open(MODE_FILE, "r");
  if (!file) {
    MyLog(ERROR, "Failed to open mode file");
    mode.useSchedule = true; // Default to schedule
    return false;
  }
  DynamicJsonDocument doc(128);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    MyLogF(ERROR, "Failed to parse mode: %s", error.c_str());
    mode.useSchedule = true; // Default to schedule
    file.close();
    return false;
  }
  mode.useSchedule = doc["use_schedule"];
  file.close();
  return true;
}


// Then declare your variables
ScheduleData defaultSchedule = {
  true,
  {"Mon", "Tue", "Wed", "Thu", "Fri", "", ""},
  5,
  "08:00",
  "20:00",
  "UTC+1"
};



// Days mapping
const char* shortDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* longDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Function Prototypes
void displaySystemStatus();
void handleButtonPress();
void syncWithServer();
bool checkSchedule(const ScheduleData& schedule);
void manageTVState();
void syncRTCWithNTP();
String generateDeviceId();
String getCurrentDay();
void updateLEDStatus();
bool parseScheduleResponse(const String& payload);
bool isDaySelected(String day);
bool initEthernet();
bool connectToNetwork();
void reportTVStateToServer();

String generateDeviceId() {
    // Try to get unique MAC address
    uint8_t baseMac[6];
    
    // For ESP32
    #if defined(ESP32)
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    #else
    // For Ethernet devices
    if (useEthernet) {
        Ethernet.MACAddress(baseMac);
    } else {
        // Fallback to WiFi MAC if available
        #if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        WiFi.macAddress(baseMac);
        #else
        // Generate a random ID if no MAC available
        for(int i = 0; i < 6; i++) {
            baseMac[i] = random(0, 255);
        }
        #endif
    }
    #endif
    
    // Create SHA256 hash of MAC address
    uint8_t hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, baseMac, sizeof(baseMac));
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    
    // Convert first 12 bytes of hash to hex string
    char id[25] = {0};
    for(int i = 0; i < 12; i++) {
        sprintf(&id[i*2], "%02X", hash[i]);
    }
    
    return String(id);
}

void updateDesiredState() {
    if (currentMode.useSchedule) {
        // Schedule mode - determine state based on current schedule
        desiredTVState = checkSchedule(currentSchedule) ? TV_ON : TV_OFF;
    }
    // In server command mode, desired state is set during syncWithServer()
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
    
    // Set timeout and retry for reliability
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
        
        // Update desired state immediately when mode changes
        updateDesiredState();
        return true;
    }
    
    return false;
}

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

void verifyTVState() {
    uint8_t hwState = checkTVStatus();
    TVState actualState = (hwState == 1) ? TV_OFF : TV_ON;
    
    if(actualState != currentTVState && currentTVState != TV_TRANSITIONING) {
        MyLogF(WARNING, "State mismatch: System=%s, Hardware=%s",
              currentTVState == TV_ON ? "ON" : "OFF",
              actualState == TV_ON ? "ON" : "OFF");
        
        currentTVState = actualState;
        if(connectToNetwork()) {
            reportTVStateToServer();
        }
    }
}

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

bool performOTAUpdate(String firmwareUrl) {
  MyLog(INFO, "Starting OTA update...");
  rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);
  
  WiFiClient client;
  
  // Configure update callbacks
  HTTPUpdateResult ret = httpUpdate.update(client, firmwareUrl);
  
  bool success = false;

  switch(ret) {
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

void printSystemStatus() {
    RtcDateTime now = rtc.RTC_getDateTime();
    String currentDay = getCurrentDay();
    uint8_t hwState = checkTVStatus();
    
    Serial.println("\n========== SYSTEM STATUS ==========");
    Serial.println("Device ID: " + DEVICE_ID);
    Serial.printf("Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                 now.Year(), now.Month(), now.Day(),
                 now.Hour(), now.Minute(), now.Second());
    Serial.println("Day: " + currentDay);
    
    Serial.printf("TV State - System: %s, Hardware: %s\n",
                 tvState.c_str(),
                 hwState ? "OFF" : "ON");
    
    Serial.printf("Operation Mode: %s\n",
                 currentMode.useSchedule ? "SCHEDULE" : "SERVER COMMAND");
    
    if(currentMode.useSchedule) {
        updateDesiredState();
        Serial.println("Schedule Status:");
        Serial.printf("Active: %s\n", currentSchedule.active ? "YES" : "NO");
        Serial.printf("Days: ");
        for(int i=0; i<currentSchedule.daysCount; i++) {
            if(i>0) Serial.print(", ");
            Serial.print(currentSchedule.days[i]);
        }
        Serial.printf("\nTime: %s to %s\n",
                     currentSchedule.startTime.c_str(),
                     currentSchedule.endTime.c_str());
        Serial.printf("Should be %s now\n",
                     checkSchedule(currentSchedule) ? "ON" : "OFF");
    } else {
        Serial.printf("Last Server Command: %s\n",
                     desiredTVState == TV_ON ? "ON" : "OFF");
    }
    
    Serial.printf("Network: %s\n",
                 WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    Serial.println("=================================");
}

#if ENABLE_OTA
bool checkForUpdates() {
  String currentVersion = loadCurrentVersion();
  MyLogF(INFO, "Current version: %s", currentVersion.c_str());

  HTTPClient http;
  String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/api/v1/esp/" + DEVICE_ID + "/firmware";
  
  if (useEthernet) {
    http.begin(serverIp.toString(), serverPort, "/api/v1/esp/" + DEVICE_ID + "/firmware");
  } else {
    http.begin(client, url);
  }

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error && doc["success"]) {
      String latestVersion = doc["data"]["latest_version"].as<String>();
      bool updateAvailable = doc["data"]["update_available"].as<bool>();
      String firmwareUrl = doc["data"]["url"].as<String>();

      MyLogF(INFO, "Latest version: %s, Update available: %d", 
            latestVersion.c_str(), updateAvailable);

      if (updateAvailable) {
        MyLog(INFO, "New version available, updating...");
        if (performOTAUpdate(firmwareUrl)) {
          saveCurrentVersion(latestVersion); // ✅ Save new version after success
          ESP.restart(); // Force restart after successful update
          return true;
        }
      } else {
        MyLog(INFO, "Firmware is up to date");
      }
    }
  } else {
    MyLogF(ERROR, "Update check failed: %d", httpCode);
  }
  http.end();
  return false;
}

#endif

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


bool pingServer() {
    if (!connectToNetwork()) {
        MyLog(INFO, "Not connected - skipping ping");
        return false;
    }

    HTTPClient http;
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/api/v1/esp/" + DEVICE_ID + "/ping";
    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, "/api/v1/esp/" + DEVICE_ID + "/ping");
    } else {
        http.begin(client, url);
    }

    http.addHeader("Content-Type", "application/json");

    // Build JSON payload
    DynamicJsonDocument doc(1024);
    doc["actual_state"] = currentTVState == TV_ON ? "on" : "off";
    doc["desired_state"] = desiredTVState == TV_ON ? "on" : "off";

    RtcDateTime rtcTime = rtc.RTC_getDateTime();
    char timeStr[20];
    sprintf(timeStr, "%04u-%02u-%02uT%02u:%02u:%02uZ",
            rtcTime.Year(), rtcTime.Month(), rtcTime.Day(),
            rtcTime.Hour(), rtcTime.Minute(), rtcTime.Second());
    doc["current_time"] = String(timeStr);

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        DynamicJsonDocument resDoc(1024);
        DeserializationError error = deserializeJson(resDoc, response);
        if (!error && resDoc["success"]) {
            JsonObject data = resDoc["data"];

            // Save returned values
            String serverMode = data["mode"].as<String>();
            currentMode.useSchedule = (serverMode == "schedule");
            saveMode(currentMode);

            desiredTVState = (data["tv_state"].as<String>() == "on") ? TV_ON : TV_OFF;
            persistentData.lastCommandedState = desiredTVState;

            currentSchedule = data["schedule"].as<ScheduleData>();
            saveSchedule(currentSchedule);

            if (data.containsKey("wifi")) { 
                WiFiConfig newConfig;
                newConfig.ssid = data["wifi"]["ssid"].as<String>();
                newConfig.password = data["wifi"]["password"].as<String>();
                
                WiFiConfig currentConfig;
                if (loadWiFiConfig(currentConfig)) {
                    if (currentConfig.ssid != newConfig.ssid || currentConfig.password != newConfig.password) {
                        if (saveWiFiConfig(newConfig)) {
                            MyLog(INFO, "Updated WiFi config from server");
                            // Reconnect if we're using WiFi
                            if (!useEthernet) {
                                network.configureWiFi(newConfig.ssid.c_str(), newConfig.password.c_str());
                                network.InitWifiModule();
                            }
                        }
                    }
                } else {
                    // No existing config, save the new one
                    if (saveWiFiConfig(newConfig)) {
                        MyLog(INFO, "Saved new WiFi config from server");
                    }
                }
            }
            
            String latestVersion = data["latest_firmware_version"].as<String>();
            String currentVersion = loadCurrentVersion();

            if (is_newer_version(currentVersion, latestVersion)) {
                MyLogF(INFO, "New firmware available: %s", latestVersion.c_str());
                if (checkForUpdates()) {
                    saveCurrentVersion(latestVersion);
                    ESP.restart();
                }
            }

            return true;
        }
    } else {
        MyLogF(ERROR, "Ping failed: %d", httpCode);
    }
    http.end();
    return false;
}

void setup() {
    Serial.begin(115200);
    pinMode(TV_STATUS_PIN, INPUT_PULLUP);
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {  // true = format if mount fails
        MyLog(ERROR, "Failed to mount LittleFS, formatting...");
        if (!LittleFS.format()) {
            MyLog(ERROR, "Failed to format LittleFS");
        } else {
            MyLog(INFO, "LittleFS formatted successfully");
            if (!LittleFS.begin()) {
                MyLog(ERROR, "Failed to mount LittleFS after formatting");
            }
        }
    } else {
        MyLog(INFO, "LittleFS mounted successfully");
        // Try to load persistent data
        loadPersistentData();
    }

    if (!rtc.begin()) {
        Serial.println("Failed to initialize RTC");
        while(1);
    }

    
    // Initialize network
    rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);
    // DEVICE_ID = generateDeviceId();
    network.setServerIp(serverIp);
    network.setSercerPort(serverPort);

    // Try Ethernet first
    if (!initEthernet()) {
        MyLog(INFO, "Falling back to WiFi");
        network.configureWiFi(ssid, password);
        network.InitWifiModule();
        useEthernet = false;
    } else {
        useEthernet = true;
    }

    // Initialize OTA
    #if ENABLE_OTA
    setupOTA();
    #endif

    checkForUpdates();

    MyLog(INFO, "----------------------------------------");
    MyLogF(INFO, "Running %s with version %s", PRODUCT_NAME, FIRMWARE_VERSION);
    MyLog(INFO, "----------------------------------------");

    // Initialize hardware
    #if ENABLE_IR
    irHandler.begin();
    irHandler.init_littleFS();
    #endif
    
    rgb.RGBInit();
    rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
    rgb.runRGBTask();
    btn.InitButton();

    // Initialize time
    configTime(3600, 0, "pool.ntp.org");
    syncRTCWithNTP();

    // Initial state
    currentTVStatus = checkTVStatus();
    tvState = currentTVStatus ? "OFF" : "ON";
    
    // Initial server sync
    syncWithServer();

    MyLog(INFO, "Setup completed successfully");


    // by aziz
   MyLog(INFO, "Starting Now ...");


   MyLog(DEBUG, "STARTING IR  ...");

   bool state = irHandler.IsIRDataavailable(f);
   if (state)
   { // isDataAvailable(settings)) {
      rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);
      MyLog(DEBUG, "COMMAND FOUND  ...");
      // irHandler.dump_data_stored(f);
      MyIRData data;
      MyLog(DEBUG, "Reading data from json file ...");
      // irHandler.DumpMyIRDataJSON(f);
      irHandler.readMyIRDataJSON(f, data);
      // MyLog(DEBUG,"Printing data  ...");
      // irHandler.PrintMyIRData(data);
      irHandler.setRecievedData(data);
      rgb.setBlinkingStatus(BRIGHT_ORANGE_COLOR_STATUS);
      
      MyLog(DEBUG, "Sending IR signal if tv off ...");
      while (!(irHandler.getTVStatus(1000) == 1 /*we use pullup*/))
      {
      MyLog(DEBUG, ".");
         irHandler.sendIR();
         // tvOff == 1
      } ;
      // Blinking_status=GR;
   }
   else
   {
      MyLog(ERROR, "No data is avalaible LITTLEFS IS EMPTY, SCANNING NOW ....");

      // Perform the scanning operation
      rgb.setBlinkingStatus(RED_COLOR_STATUS);

      irHandler.receiveIR(true);
      rgb.setBlinkingStatus(BRIGHT_ORANGE_COLOR_STATUS);
      MyLog(DEBUG, "Sending IR signal if tv off ...");
    
       while (!(irHandler.getTVStatus(1000) == 1 /*we use pullup*/))
      {
         MyLog(DEBUG, ".");
     
        irHandler.sendIR();
         // tvOff == 1
      };
      

   }
    MyLog(INFO, "Finished Sending IR data");
   rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
   

}
    static unsigned long lastStateVerify = 0;

void loop() {
     btn.HandleButton_5s();
   if (btn.isLongPressed_5s())
   {
      MyLog(DEBUG, "BUtton PRESSD for more than 5s ....");
      rgb.setBlinkingStatus(RED_COLOR_STATUS);
      // clearr the json file that holds the IR data
      irHandler.clear_file(f);
      rgb.setBlinkingStatus(VIOLET_COLOR_STATUS);
      irHandler.receiveIR(true);
      rgb.setBlinkingStatus(BRIGHT_ORANGE_COLOR_STATUS);
      MyLog(DEBUG, "Sending IR signal if tv off ...");
      while (!(irHandler.getTVStatus(1000) == 1 /*we use pullup*/))
      {  
        MyLog(DEBUG, ".");

        irHandler.sendIR();
         // tvOff == 1
      } ;
      MyLog(INFO, "Finished Sending IR data ");
   rgb.setBlinkingStatus(GREEN_COLOR_STATUS);

      //btime = millis();

      // OPT restart the ESP
       //ESP.restart();
    }else if(millis() - lastStateVerify > 60000) { // Every 1 minute
      MyLog(DEBUG, "Checking TV state ...");
      lastStateVerify = millis();
    
     while (!(irHandler.getTVStatus(1000) == 1 /*we use pullup*/))
      {
        MyLog(DEBUG, "Sending IR signal"); 
        irHandler.sendIR();
         // tvOff == 1
      } ;
     // MyLog(INFO, "Finished Sending IR data");
   rgb.setBlinkingStatus(WHITE_COLOR_STATUS);
   }

  // rgb.setBlinkingStatus(WHITE_COLOR_STATUS);
/*  static unsigned long lastStatusPrint = 0;
    static unsigned long lastSyncTime = 0;
    static unsigned long lastStateCheck = 0;
    static unsigned long lastPingTime = 0;
    
    #if ENABLE_OTA
    otaServer.handleClient();
    #endif

    handleButtonPress();

    // Print detailed status every 20 seconds
    if(millis() - lastStatusPrint >= 20000) {
        printSystemStatus();
        lastStatusPrint = millis();
    }

    static unsigned long lastStateVerify = 0;
    if(millis() - lastStateVerify > 30000) { // Every 30 seconds
        verifyTVState();
        lastStateVerify = millis();
    }

    // Check and manage TV state every 30 seconds
    if(millis() - lastStateCheck >= 30000) {
        manageTVState();
        lastStateCheck = millis();
    }

    // Ping server every 30 seconds
    if(millis() - lastPingTime >= 30000) {
        pingServer();
        lastPingTime = millis();
    }

    // Sync with server more frequently when in server command mode
    if(connectToNetwork()) {
        unsigned long syncInterval = currentMode.useSchedule ? 60000 : 15000;
        if(millis() - lastSyncTime >= syncInterval) {
            syncWithServer();
            lastSyncTime = millis();
        }
    }

    updateLEDStatus();
    delay(50);*/
  
}

void updateLEDStatus() {
  // Update LED based on current state
  if (WiFi.status() != WL_CONNECTED) {
    rgb.setBlinkingStatus(RED_COLOR_STATUS);
  } else if (tvState == "ON") {
    rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
  } else {
    rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
  }
}

// Function Prototypes
void displaySystemStatus() {
    static unsigned long lastDisplayTime = 0;
    if (millis() - lastDisplayTime > 5000) {
        lastDisplayTime = millis();
        
        RtcDateTime now = rtc.RTC_getDateTime();
        String currentDay = getCurrentDay();
        
        Serial.println("\n===== SYSTEM STATUS =====");
        Serial.print("Time: ");
        Serial.print(now.Year()); Serial.print("-");
        Serial.print(now.Month()); Serial.print("-");
        Serial.print(now.Day()); Serial.print(" ");
        Serial.print(now.Hour()); Serial.print(":");
        Serial.print(now.Minute()); Serial.print(":");
        Serial.println(now.Second());
        
        Serial.print("Day: ");
        Serial.println(currentDay);
        
        Serial.print("TV State: ");
        // Serial.print(tvState);
        // Serial.print(" (HW: ");
        Serial.print(checkTVStatus() ? "OFF" : "ON");
        tvState = checkTVStatus() ;
        
        Serial.print("Schedule: ");
        Serial.print(currentSchedule.active ? "Active" : "Inactive");
        Serial.print(" (");
        Serial.print(currentSchedule.startTime); 
        Serial.print(" to "); 
        Serial.print(currentSchedule.endTime);
        Serial.print(") Days: ");
        for (int i = 0; i < currentSchedule.daysCount; i++) {
            if (i > 0) Serial.print(", ");
            Serial.print(currentSchedule.days[i]);
        }
        Serial.println();
        
        Serial.print("WiFi: ");
        Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        Serial.println("=======================");
    }
}

// Enhanced button handling
void handleButtonPress() {
  static unsigned long lastPressTime = 0;
  static bool inLearningMode = false;
  
  btn.HandleButton_5s();
  
  if (btn.isLongPressed_5s() && !inLearningMode) {
    inLearningMode = true;
    lastPressTime = millis();
    MyLog(INFO, "Button long press detected");
    rgb.setBlinkingStatus(NO_COLOR_STATUS);
    
    #if ENABLE_IR
    MyLog(INFO, "Entering IR learning mode");
    irHandler.clear_file(IR_DATA_FILE);
    bool success = irHandler.receiveIR(true);

    if (success) {
      MyLog(INFO, "IR learning successful");

    } else {
      MyLog(ERROR, "IR learning failed");
    }
    #else
    MyLog(INFO, "IR learning mode would activate here (module disabled)");
    delay(2000); // Simulate learning time
    #endif
    
    inLearningMode = false;
    rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
  }
}




void syncWithServer() {
    static unsigned long lastSuccessfulSync = 0;
    const unsigned long SYNC_VALID_DURATION = 86400000; // 24 hours
    static unsigned long lastSyncTime = 0;
    static unsigned long lastScheduleUpdate = 0;

    bool isOnline = connectToNetwork();

    // If offline and we have recent data, use that
    if (!isOnline && millis() - persistentData.lastUpdate < SYNC_VALID_DURATION) {
        MyLog(INFO, "Offline - using cached data");
        currentMode = persistentData.mode;
        currentSchedule = persistentData.schedule;

        if (!currentMode.useSchedule) {
            desiredTVState = persistentData.lastCommandedState;
        } else {
            updateDesiredState(); // Update based on schedule
        }
        return;
    }

    if (!isOnline) {
        MyLog(INFO, "Offline and no recent data - skipping sync");
        return;
    }

    // Main Sync Logic
    if (millis() - lastSyncTime > SYNC_INTERVAL || lastSyncTime == 0) {
        MyLog(INFO, "Performing scheduled sync...");
        reportTVStateToServer(); // Optional
        pingServer(); // This handles everything else
        lastSyncTime = millis();
    }

    // 2. Always fetch the current mode
    if (fetchModeFromServer()) {
        persistentData.mode = currentMode;
        persistentData.lastUpdate = millis();
        savePersistentData();
    }

     // 3. Handle WiFi config updates from server
    HTTPClient http;
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/api/v1/esp/" + DEVICE_ID + "/wifi"; // Changed endpoint
    
    if (useEthernet) {
        http.begin(serverIp.toString(), serverPort, "/api/v1/esp/" + DEVICE_ID + "/wifi");
    } else {
        http.begin(client, url);
    }

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc["success"]) {
            WiFiConfig newConfig;
            newConfig.ssid = doc["data"]["ssid"].as<String>();
            newConfig.password = doc["data"]["password"].as<String>();

            WiFiConfig currentConfig;
            bool hasCurrentConfig = loadWiFiConfig(currentConfig);
            
            // Only update if different from current config
            if (!hasCurrentConfig || 
                (currentConfig.ssid != newConfig.ssid || currentConfig.password != newConfig.password)) {
                
                if (saveWiFiConfig(newConfig)) {
                    MyLog(INFO, "New WiFi config saved");
                    
                    // Reconnect if we're using WiFi
                    if (!useEthernet) {
                        MyLog(INFO, "Reconnecting with new WiFi credentials");
                        network.configureWiFi(newConfig.ssid.c_str(), newConfig.password.c_str());
                        network.InitWifiModule();
                    }
                }
            }
        } else {
            if (error) {
                MyLogF(ERROR, "Failed to parse WiFi config: %s", error.c_str());
            }
        }
    } else {
        MyLogF(ERROR, "Failed to get WiFi config: %d", httpCode);
    }
    http.end();


    // 4. Handle state based on current mode
    if (!currentMode.useSchedule) {
        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/api/v1/esp/" + DEVICE_ID + "/tv";

        if (useEthernet) {
            http.begin(serverIp.toString(), serverPort, url);
        } else {
            http.begin(client, url);
        }

        http.setTimeout(5000);
        http.setReuse(true);

        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            MyLogF(INFO, "Server response: %s", payload.c_str());

            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error && doc["success"]) {
                String serverCommand = doc["data"]["commanded_state"].as<String>();
                bool shouldUpdate = doc["should_update"].as<bool>();

                MyLogF(INFO, "Server command: %s, should update: %d", 
                      serverCommand.c_str(), shouldUpdate);

                desiredTVState = (serverCommand == "on") ? TV_ON : TV_OFF;
                persistentData.lastCommandedState = desiredTVState;
                persistentData.lastUpdate = millis();
                savePersistentData();

                if (shouldUpdate) {
                    MyLogF(INFO, "Updated desired state to: %s", 
                          desiredTVState == TV_ON ? "ON" : "OFF");
                }
            } else {
                if (error) {
                    MyLogF(ERROR, "JSON parse error: %s", error.c_str());
                } else {
                    MyLog(ERROR, "Server reported failure");
                }
            }
        } else {
            MyLogF(ERROR, "HTTP error: %d", httpCode);
        }
        http.end();
    }
    // 5. Check for schedule updates (less frequent)
    if (millis() - lastScheduleUpdate > 300000) { // Every 5 minutes
        if (updateScheduleFromServer()) {
            persistentData.schedule = currentSchedule;
            persistentData.lastUpdate = millis();
            savePersistentData();
            lastScheduleUpdate = millis();
            updateDesiredState();
        }
    }

    lastSuccessfulSync = millis();
}


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


void manageTVState() {
    #if ENABLE_IR
    static unsigned long lastActionTime = 0;
    const unsigned long ACTION_COOLDOWN = 20000; // 20 seconds between actions
    
    // Get current hardware state
    uint8_t hwState = checkTVStatus();
    TVState actualState = (hwState == 1) ? TV_OFF : TV_ON;
    tvState = actualState == TV_ON ? "ON" : "OFF";
    
    // Update current state if not transitioning
    if(currentTVState != TV_TRANSITIONING) {
        currentTVState = actualState;
    }
    
    // Skip if in cooldown period
    if(millis() - lastActionTime < ACTION_COOLDOWN) {
        return;
    }
    
    // Determine if we need to change state
    verifyTVState();
    hwState = checkTVStatus();
    TVState currentTVState = (hwState == 1) ? TV_OFF : TV_ON;
    bool stateNeedsChange = (currentTVState != desiredTVState);
    
    // Debug output
    MyLogF(INFO, "TV State Check: Current=%s, Desired=%s, HW=%s, Change=%d", 
          currentTVState == TV_ON ? "ON" : "OFF",
          desiredTVState == TV_ON ? "ON" : "OFF",
          actualState == TV_ON ? "ON" : "OFF",
          stateNeedsChange);
    
    // Only proceed if state needs changing
    if(!stateNeedsChange) {
        return;
    }
    
    // State needs to be changed
    MyLogF(INFO, "TV State Change: Current=%s, Desired=%s, Mode=%s",
          currentTVState == TV_ON ? "ON" : "OFF",
          desiredTVState == TV_ON ? "ON" : "OFF",
          currentMode.useSchedule ? "schedule" : "server");
    
    // Mark as transitioning and send IR command
    currentTVState = TV_TRANSITIONING;
    lastActionTime = millis();
    irHandler.sendIR();
    
    // Verify after delay
    delay(20000); // Wait for TV to respond
    
    // Check new state
    hwState = checkTVStatus();
    TVState newState = (hwState == 1) ? TV_OFF : TV_ON;
    
    if(newState == desiredTVState) {
        currentTVState = newState;
        MyLog(INFO, "TV state changed successfully");
        if(connectToNetwork()) {
            reportTVStateToServer();
        }
    } else {
        MyLog(ERROR, "Failed to change TV state");
        currentTVState = actualState; // Revert to known state
    }
    #endif
}



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

void syncRTCWithNTP() {
    if (!rtc.begin()) {
        MyLog(ERROR, "RTC initialization failed!");
        return;
    }

    // Configure NTP
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10000)) { // 10 second timeout
        MyLog(WARNING, "Failed to get NTP time");
        return;
    }

    // Calculate day of week (0=Sunday, 6=Saturday)
    uint8_t dayOfWeek = timeinfo.tm_wday;
    if (dayOfWeek > 6) { // If invalid, calculate it
        // Zeller's Congruence algorithm
        int d = timeinfo.tm_mday;
        int m = timeinfo.tm_mon + 1;
        int y = 1900 + timeinfo.tm_year;
        
        if (m < 3) {
            m += 12;
            y--;
        }
        
        dayOfWeek = (d + (13 * (m + 1) / 5) + y + (y / 4) - (y / 100) + (y / 400)) % 7;
    }

    // Convert to RtcDateTime (without day of week)
    RtcDateTime ntpTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );
    
    // Set date/time first
    rtc.setDateTime(ntpTime);
    
    // Then set day of week separately for DS1307
    #ifdef RTC_DS1307
    // DS1307 uses 1=Sunday through 7=Saturday
    uint8_t ds1307Day = (dayOfWeek + 1) % 7;
    if (ds1307Day == 0) ds1307Day = 7;
    
    Wire.beginTransmission(0x68); // DS1307 I2C address
    Wire.write(0x03); // Day of week register
    Wire.write(ds1307Day);
    Wire.endTransmission();
    #endif
    
    // Verify
    RtcDateTime rtcTime = rtc.RTC_getDateTime();
    MyLogF(INFO, "RTC synced: %04d-%02d-%02d %02d:%02d:%02d",
          rtcTime.Year(), rtcTime.Month(), rtcTime.Day(),
          rtcTime.Hour(), rtcTime.Minute(), rtcTime.Second());
}
