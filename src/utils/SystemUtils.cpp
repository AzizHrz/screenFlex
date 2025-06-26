#include "utils/SystemUtils.h"
#include "utils/Mylog.h"

// 2

#include "dataModel.h"
#include <RtcDateTime.h>
#include "utils/ScheduleUtils.h"
#include "utils/ModeUtils.h"
#include "network/NetworkUtils.h"
#include <mbedtls/md.h>
#include <WiFi.h>
#include <Ethernet.h>

// 1
//extern int TV_STATUS_PIN;
#include "Hardware.h"

uint8_t checkTVStatus() {
    static uint8_t lastStableState = 0;
    static uint8_t stableCount = 0;
    const uint8_t requiredStableReadings = 5;
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

    return 2; // unstable
}

bool is_newer_version(const String& current, const String& available) {
    int c_major, c_minor, c_patch;
    int a_major, a_minor, a_patch;

    sscanf(current.c_str(), "%d.%d.%d", &c_major, &c_minor, &c_patch);
    sscanf(available.c_str(), "%d.%d.%d", &a_major, &a_minor, &a_patch);

    if (a_major > c_major) return true;
    if (a_major == c_major && a_minor > c_minor) return true;
    return (a_major == c_major && a_minor == c_minor && a_patch > c_patch);
}

// 2
// SystemUtils.cpp (extend it)


//extern RtcDS1307 rtc;
extern RTC_Module rtc;


extern String DEVICE_ID;
extern ScheduleData currentSchedule;
extern ModeData currentMode;
extern TVState currentTVState;
extern TVState desiredTVState;
extern String tvState;
extern uint8_t checkTVStatus();
extern String getCurrentDay();



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
                 tvState.c_str(), hwState ? "OFF" : "ON");

    Serial.printf("Operation Mode: %s\n", currentMode.useSchedule ? "SCHEDULE" : "SERVER COMMAND");

    if (currentMode.useSchedule) {
        updateDesiredState();
        Serial.println("Schedule Status:");
        Serial.printf("Active: %s\n", currentSchedule.active ? "YES" : "NO");
        Serial.printf("Days: ");
        for (int i = 0; i < currentSchedule.daysCount; i++) {
            if (i > 0) Serial.print(", ");
            Serial.print(currentSchedule.days[i]);
        }
        Serial.printf("\nTime: %s to %s\n",
                     currentSchedule.startTime.c_str(),
                     currentSchedule.endTime.c_str());
        Serial.printf("Should be %s now\n",
                     checkSchedule(currentSchedule) ? "ON" : "OFF");
    } else {
        Serial.printf("Last Server Command: %s\n", desiredTVState == TV_ON ? "ON" : "OFF");
    }

    Serial.printf("Network: %s\n", WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    Serial.println("=================================");
}

String generateDeviceId() {
    uint8_t baseMac[6];

    #if defined(ESP32)
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    #else
    if (useEthernet) {
        Ethernet.MACAddress(baseMac);
    } else {
        #if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        WiFi.macAddress(baseMac);
        #else
        for (int i = 0; i < 6; i++) {
            baseMac[i] = random(0, 255);
        }
        #endif
    }
    #endif

    uint8_t hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, baseMac, sizeof(baseMac));
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    char id[25] = {0};
    for (int i = 0; i < 12; i++) {
        sprintf(&id[i * 2], "%02X", hash[i]);
    }

    return String(id);
}

