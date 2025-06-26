#include "modules/RTC_module.h"
#include <Wire.h>
#include "RtcDS1307.h"

// DS1307 uses standard I2C pins (no ThreeWire needed)
#define I2C_SDA 21
#define I2C_SCL 22



void RTC_Module::RTC_Timer(uint32_t seconds) {
    duration_seconds = seconds;
    start_time = RtcDateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), rtc.now().hour(), rtc.now().minute(), rtc.now().second());
}

void RTC_Module::RTC_Module_init() {
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    if (!begin()) {
        Serial.println("RTC initialization failed!");
        return;
    }

    DateTime compiled = DateTime(F(__DATE__), F(__TIME__));
    printDateTime(RtcDateTime(compiled.year(), compiled.month(), compiled.day(), compiled.hour(), compiled.minute(), compiled.second()));
    Serial.println();

    DateTime now = rtc.now();
    
    Serial.println("==================================");
    Serial.println("Comparing times");
    Serial.println("==================================");
    Serial.println("Current time ");
    printDateTime(RtcDateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()));
    Serial.println("Compiled Time ");
    printDateTime(RtcDateTime(compiled.year(), compiled.month(), compiled.day(), compiled.hour(), compiled.minute(), compiled.second()));
    Serial.println("====================================");

    if (now < compiled) {
        Serial.println("RTC is older than compile time! (Updating DateTime)");
        rtc.adjust(compiled);
    } else if (now > compiled) {
        Serial.println("RTC is newer than compile time. (this is expected)");
    } else {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
}

void RTC_Module::printDateTime(const RtcDateTime& dt) {
    char datestring[20];
    snprintf_P(datestring, sizeof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
    Serial.println(datestring);
}

bool RTC_Module::isElapsed() {
    DateTime current_time = rtc.now();
    uint32_t elapsed_seconds = ((current_time.year() * 365 * 24 * 3600 + current_time.month() * 30 * 24 * 3600 + current_time.day() * 24 * 3600 +
                                 current_time.hour() * 3600 + current_time.minute() * 60 + current_time.second()) -
                                (start_time.Year() * 365 * 24 * 3600 + start_time.Month() * 30 * 24 * 3600 + start_time.Day() * 24 * 3600 +
                                 start_time.Hour() * 3600 + start_time.Minute() * 60 + start_time.Second()));
    return elapsed_seconds >= duration_seconds;
}

RtcDateTime RTC_Module::RTC_getDateTime() {
    DateTime now = rtc.now();
    if (!now.isValid()) {
        Serial.println("Error: RTC returned invalid time");
    }
    return RtcDateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
}

bool RTC_Module::RTC_SetTimer_min() {
    int duration = 10; // seconds
    DateTime start = rtc.now();
    start_min = (int)start.second();
    
    if (timer == false) {
        prev_min = start_min + duration;
        if (prev_min >= 60) {
            prev_min = prev_min - 60;
            time_func = true;
        }
        timer = true;
    }
    
    if (start_min == 0) {
        time_func = false;
    }
    
    Serial.println(time_func);
    Serial.println(prev_min);
    Serial.println(start.second(), DEC);
    
    if (!time_func && start_min >= prev_min) {
        Serial.println("Stopping timer now");
        prev_min = 0;
        return false;
    } else if (time_func && start_min <= prev_min) {
        Serial.println("Stopping timer now");
        prev_min = 0;
        time_func = false;
        return false;
    }

    return true;
}

// Rest of the methods remain the same as they use RtcDateTime which is compatible
void RTC_Module::set_beginTime(RtcDateTime beginTime) {
    _beginTime = beginTime;
}

void RTC_Module::set_endTime(RtcDateTime endTime) {
    _endTime = endTime;
}

void RTC_Module::setDateTime(RtcDateTime dateTime) {
    DateTime dt(dateTime.Year(), dateTime.Month(), dateTime.Day(), dateTime.Hour(), dateTime.Minute(), dateTime.Second());
    rtc.adjust(dt);
}

bool RTC_Module::isWithinOperatingPeriod(RtcDateTime currentTime, bool isWithinSchedule) {
    return (currentTime.Hour()*60 + currentTime.Minute() >= _beginTime.Hour()*60 + _beginTime.Minute() && 
            currentTime.Hour()*60 + currentTime.Minute() <= _endTime.Hour()*60 + _endTime.Minute()) && 
            isWithinSchedule;
}

String RTC_Module::rtcDateTimeToString(const RtcDateTime& dateTime) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             dateTime.Year(), dateTime.Month(), dateTime.Day(),
             dateTime.Hour(), dateTime.Minute(), dateTime.Second());
    return String(buffer);
}

RtcDateTime RTC_Module::convertToRtcDateTime(const String &timeString) {
    int year = timeString.substring(0, 4).toInt();
    int month = timeString.substring(5, 7).toInt();
    int day = timeString.substring(8, 10).toInt();
    int hour = timeString.substring(11, 13).toInt();
    int minute = timeString.substring(14, 16).toInt();
    int second = timeString.substring(17, 19).toInt();

    return RtcDateTime(year, month, day, hour, minute, second);
}