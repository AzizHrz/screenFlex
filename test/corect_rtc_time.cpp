#include <Arduino.h>
#include "RTC_module.h"

RTC_Module RTC_mod;

void setRTCTimeFromPC() {
    // Extract the compile time from the PC
    const char *dateStr = __DATE__;  // Format: "Jan 29 2025"
    const char *timeStr = __TIME__;  // Format: "12:34:56"

    int month, day, year, hour, minute, second;
    char monthStr[4];

    sscanf(dateStr, "%s %d %d", monthStr, &day, &year);
    sscanf(timeStr, "%d:%d:%d", &hour, &minute, &second);

    // Convert month name to number
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    for (month = 0; month < 12; month++) {
        if (strncmp(monthStr, months[month], 3) == 0) {
            month += 1;  // Months in RTCDateTime start from 1
            break;
        }
    }

    // Set the RTC time
    RtcDateTime newTime(year, month, day, hour, minute, second);
    RTC_mod.RTC_setDateTime(newTime);
    Serial.println("RTC Time Updated from PC!");
}

void setup() {
    Serial.begin(115200);
    RTC_mod.RTC_Module_init();

    // Uncomment this line, upload once, then recomment it and upload again
    setRTCTimeFromPC();
}

void loop() {
    Serial.println("Hello World from OTA");

    RtcDateTime dt = RTC_mod.RTC_getDateTime();
    RTC_mod.printDateTime(dt);

    delay(500);
}
