// /*
//  * Software Verision 1.2.1 , Moktar SELLAMI
//  * @screen_flex
//  */

#include "conf.h"
#include "soft_version.h"
#include "network_stack.h"
#include "Hardware.h"
#include "RGBModule.h"
#include "ButtonModule.h"
//#include "IR_Module.h"
#include "IRModule.h"
//#include "RTC_module.h"
#include "RTCModule.h"
#include "Mylog.h"
#include <Arduino.h>

#include <HTTPUpdate.h> // Include the ESP32 HTTP Update library



//IRHandler irHandler(IR_RECEIVE_PIN, IR_SEND_PIN_, TV_STATUS_PIN);
IRHandler irHandler(IR_RECEIVE_PIN, 32, TV_STATUS_PIN);
ButtonModule btn(PIN_BTN);
RGBModule rgb(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);
NetworkStack network;
RTC_Module rtc;
//volatile uint8_t Blinking_status = BLUE_COLOR_STATUS; // 0: Red, 1: Green, 2: Blue
volatile bool Wake_PC_STATE = false;
volatile bool Wake_PC_STATE_finished = false;
// pin defs

unsigned long btime = 0;

static unsigned long lastStateVerify = 0;
static unsigned long lastPing = 0;


RtcDateTime nowTime;

// ethnet configuration
bool useEthernet = false;
// WiFi configuration
const char* ssid = "Aziz_s_iPhone";
const char* passwd = "just100$";

struct WiFiConfig {
  String ssid;
  String password;
};

//==============================================================

EthernetClient ethClient;
WiFiClient client;

IPAddress serverIp(18 ,235, 73, 121); // Change to your Flask server IP
const int serverPort = 5000;

// TV State Enumeration - Add this before PersistentData struct
enum TVState {
  TV_UNKNOWN,      // Initial/unknown state
  TV_OFF,          // Confirmed OFF state
  TV_ON,           // Confirmed ON state
  TV_TRANSITIONING // Currently changing state
};
// Global State Variables
TVState currentTVState = TV_UNKNOWN;
TVState desiredTVState = TV_UNKNOWN;

bool scheduleModeEnabled; // true = use schedule, false = use server commands
bool currentMode; // true = use schedule, false = use server commands

// Then declare your PersistentData struct
struct PersistentData {
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
//#define IR_DATA_FILE "/ir_data.json"
//#define VERSION_FILE "/version.json"

String DEVICE_ID = "SOLUTIONGENERIQUE1";
/*
// the old way to store the schedule data
struct ScheduleData {
  bool active;
  //bool dayEnabled[7]; // true for each day of the week
  String days[7];
  int daysCount;
  String startTime;
  String endTime;
  String timezone;
};
ScheduleData currentSchedule;

void convertFromJson(JsonVariantConst src, ScheduleData& dst) { // Convert JSON variant to ScheduleData struct
   // to avoid this error
   // "lib/ArduinoJson/src/ArduinoJson/Variant/ConverterImpl.hpp:24:20: error: no matching function
   // for call to 'convertFromJson(ArduinoJson6201_F1::JsonVariantConst&, ScheduleData&)'
   //  convertFromJson(src, result);  // Error here? See https://arduinojson.org/v6/unsupported-as/"
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
*/
struct TimePeriod {
  String startTime;
  String endTime;
};

struct ScheduleDay {
  std::vector<TimePeriod> periods;  // Multiple time periods for one day
};

struct ScheduleData {
  bool active;
  bool dayEnabled[7];  // Sunday=0, ..., Saturday=6
  ScheduleDay days[7]; // One ScheduleDay per weekday
  String timezone;
};

ScheduleData currentSchedule;

// Function to convert JSON variant to ScheduleData struct
void convertFromJson(JsonVariantConst src, ScheduleData& dst) {
  dst.active = src["active"] | dst.active;
  dst.timezone = src["timezone"] | dst.timezone;

  // Initialize all days as disabled
  for (int i = 0; i < 7; i++) {
    dst.dayEnabled[i] = false;
    dst.days[i].periods.clear();
  }

  // Read the "weekdays" array from JSON
  JsonArrayConst weekdays = src["weekdays"];
  for (JsonObjectConst dayEntry : weekdays) {
    int dayIndex = dayEntry["day"] | -1;
    if (dayIndex >= 0 && dayIndex < 7) {
      dst.dayEnabled[dayIndex] = true;

      JsonArrayConst periods = dayEntry["periods"];
      for (JsonObjectConst period : periods) {
        TimePeriod p;
        p.startTime = period["start_time"] | "";
        p.endTime = period["end_time"] | "";
        dst.days[dayIndex].periods.push_back(p);
      }
    }
  }
}


bool saveMode(const bool& scheduleModeEnabled) {
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
  doc["use_schedule"] = scheduleModeEnabled;
  size_t bytesWritten = serializeJson(doc, file);
  file.close();
  if (bytesWritten == 0) {
    MyLog(ERROR, "Failed to write mode");
    return false;
  }
  MyLogF(INFO, "Mode saved (%d bytes)", bytesWritten);
  return true;
}

bool loadMode(bool& scheduleModeEnabled) { // Load the mode from LittleFS
  if (!LittleFS.exists(MODE_FILE)) {
    MyLog(INFO, "No mode file, using default (schedule)");
    scheduleModeEnabled = true; // Default to schedule
    return false;
  }
  File file = LittleFS.open(MODE_FILE, "r");
  if (!file) {
    MyLog(ERROR, "Failed to open mode file");
    scheduleModeEnabled = true; // Default to schedule
    return false;
  }
  DynamicJsonDocument doc(128);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    MyLogF(ERROR, "Failed to parse mode: %s", error.c_str());
    scheduleModeEnabled = true; // Default to schedule
    file.close();
    return false;
  }
  scheduleModeEnabled = doc["use_schedule"];
  file.close();
  return true;
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

bool connectToNetwork() {
    //network.InitEthernetModule(W5500_CS);
   // by aziz
   
   if(!network.InitEthernetModule(W5500_CS)) {
      // we chech 3 times if the ethernet module is initialized 
      MyLog(INFO, "Failed to initialize Ethernet module");
       MyLog(INFO, "Falling back to WiFi");
//        network.configureWiFi(ssid, password);
        network.configureWiFi(ssid, passwd);

        network.InitWifiModule();
        useEthernet = false;
        return false;
   } else {
      MyLog(INFO, "Ethernet module initialized successfully");
        useEthernet = true;
        return true;
   }
/*
// ayoub logic 
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

*/
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

bool pingServer() {
    // Check network connection first
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
            scheduleModeEnabled = (serverMode == "schedule");
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

//============================================================================
void setup()
{
   Serial.begin(115200);
   MyLog(INFO, "----------------------------------------");
   MyLogF(INFO, "Runing %s with version %s", PRODUCT_NAME, FIRMWARE_VERSION);
   MyLog(INFO, "----------------------------------------");
   
   
   if (!rtc.begin()) {
        //Serial.println("Failed to initialize RTC");
        MyLog(INFO, "Failed to initialize RTC");

        while(1);
    }

   // by aziz
  // Serial.println("============ Time Debug ============");
        MyLog(INFO, "============ Time Debug ============");
   
   //  Serial.print("RTC Current Time: ");
        MyLog(INFO, "RTC Current Time: ");

   nowTime = rtc.RTC_getDateTime();
   Serial.println(rtc.rtcDateTimeToString(nowTime));

     
   // init storege and RGB LED
   irHandler.begin();
   irHandler.init_littleFS(); // TODO: add option to clear FS

   // init RGB
   rgb.RGBInit();
   rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
   // init_RGB();
   // init Button 12
   btn.InitButton();
   // init tasks
   rgb.runRGBTask();
   /////////////////////////////////////////////////////////////
   rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);
   
   /*
   //by ayoub 
   // Try Ethernet first
    if (!initEthernet()) {
        MyLog(INFO, "Falling back to WiFi");
        network.configureWiFi(ssid, password);
        network.InitWifiModule();
        useEthernet = false;
    } else {
        useEthernet = true;
    }
  */
   
   static bool connected =  connectToNetwork();
   rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
   
   
   // initize the ethernet Module
   
   MyLog(INFO, "Finished Initializing all the Modules  ...");
   MyLog(INFO, "Starting Now ...");
/* delay(50);
   MyLog(DEBUG, "STARTING WOL TASK ...");
   Wake_PC_STATE = false;
   network.initTaskWol();
*/
  
   
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
//btime = millis();
   /*// OTA handlin ...
#if (ENABLE_OTA == 1)
   IPAddress ip SERVER_IP;
   network.setServerIp(ip);
   network.setSercerPort(SERVER_PORT);
   network.configureWiFi(WIFI_SSID, WIFI_PASSWD);
   network.InitWifiModule();
   MyLog(INFO, "Performing OTA ");
   network.configureOTA(true);

   bool st = network.OTA();
   if (st)
   {
      MyLog(INFO, "RESTATRING OTA ");
      ESP.restart();
   }
   else
   {
      MyLog(INFO, " OTA Failed miserably ");
   }
#else
   MyLog(INFO, " OTA is disabled ");

#endif*/
   
}
void loop()
{
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
        rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);

        MyLog(DEBUG, "Sending IR signal"); 
        irHandler.sendIR();
         // tvOff == 1
      } ;
     // MyLog(INFO, "Finished Sending IR data");
   rgb.setBlinkingStatus(WHITE_COLOR_STATUS);

   // by aziz
   Serial.println("======== Time Comparison Debug ========");

   Serial.print("RTC Current Time: ");
   nowTime = rtc.RTC_getDateTime();
   Serial.println(rtc.rtcDateTimeToString(nowTime));
   if(millis() - lastPing > 30000){
      if (connectToNetwork()){
         MyLog(INFO, "Connected to network, pinging server ...");
         if (pingServer()) {
            MyLog(INFO, "Ping successful, processing server response ...");
            // Process server response here
         } else {
            MyLog(ERROR, "Ping failed or no valid response from server");
         }
      } else {
         MyLog(ERROR, "Failed to connect to network");
      }
   }

   }
   
}
