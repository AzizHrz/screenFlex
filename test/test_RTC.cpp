#include <Arduino.h>
#include "RTC_module.h"
#include "Hardware.h"
RTC_Module rtc(RTC_DAT,RTC_RST,RTC_CLK);
RtcDateTime now ;

RtcDateTime beginTime(2025, 1, 24, 16, 14, 0);  
RtcDateTime endTime(2025, 1, 24, 16, 16, 0); 

void setup(){ 
    
    rtc.RTC_Module_init();
    rtc.checkOperatingPeriod();
    rtc.set_beginTime(beginTime);
    rtc.set_endTime(endTime);

}
void loop(){ 


    now = rtc.RTC_getDateTime();
    rtc.checkOperatingPeriod();
    rtc.isWithinOperatingPeriod(now);
    rtc.printDateTime(now);
    rtc.checkOperatingPeriod();
    delay(60000);

}
