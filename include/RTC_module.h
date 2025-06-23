#ifndef RTC_MODULE_H
#define RTC_MODULE_H

// #include <Rtc1307.h> // Removed as RTClib.h already includes DS1307 support
#include <Wire.h>
#include <RTClib.h>  // This already includes RtcDS1307 support
#include <RtcDateTime.h>

class RTC_Module {
private:
    RTC_DS1307 rtc;  // Changed to DS1307 specific
    RtcDateTime _beginTime;
    RtcDateTime _endTime;
    int start_min;
    int prev_min;
    bool timer = false;
    bool time_func = false;
    uint32_t duration_seconds;
    RtcDateTime start_time; // Variable to store the start time

public:
    RTC_Module() : start_min(0), prev_min(0), time_func(false), rtc() {}
    
    // Initialize RTC with proper DS1307 setup
    bool begin() {
        if (!rtc.begin()) {
            return false;
        }
        if (!rtc.isrunning()) {
            // Optional: Set to compile time if RTC lost power
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        return true;
    }

    // Keep all your existing methods as they are:
    void RTC_Timer(uint32_t seconds);
    bool isRtcRunning();
    bool initRTC();
    void RTC_Module_init();
    void printDateTime(const RtcDateTime& dt);
    RtcDateTime RTC_getDateTime();
    bool isElapsed();
    bool RTC_SetTimer_min();
    uint8_t DayOfWeek();
    void set_beginTime(RtcDateTime beginTime);
    void set_endTime(RtcDateTime endTime);
    void setDateTime(RtcDateTime dateTime);
    bool isWithinOperatingPeriod(RtcDateTime currentTime, bool isWithinSchedule);
    String rtcDateTimeToString(const RtcDateTime& dateTime);
    RtcDateTime convertToRtcDateTime(const String &timeString);
};

#endif