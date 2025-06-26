#include <Arduino.h>
#include "config.h"
#include "soft_version.h"
#include "network_stack.h"
#include "Hardware.h"
#include "RGBModule.h"
#include "ButtonModule.h"
#include "IR_Module.h"
#include "Mylog.h"

// newww
#include "dataModel.h"

#include "OTAUpdate.h"



#include "RTC_module.h"

#include "ScheduleUtils.h"

// =========================== 1 ===========================

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

//============================================================================

// =========================== 2 ===========================

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
//============================================================================

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

}

void loop() {

    static unsigned long lastStatusPrint = 0;
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
    delay(50);
}
