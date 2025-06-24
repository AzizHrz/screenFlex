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
