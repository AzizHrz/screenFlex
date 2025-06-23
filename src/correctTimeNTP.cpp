#include <ThreeWire.h>
#include <RtcDS1302.h>

// DS1302 Pins (change these to match your wiring)
#define RTC_DAT 21
#define RTC_CLK 22
#define RTC_RST 14

ThreeWire myWire(RTC_DAT, RTC_CLK, RTC_RST);
RtcDS1302<ThreeWire> rtc(myWire);

void printTime();

void setup() {
  Serial.begin(115200);
  
  rtc.Begin();

  // HARDCODED TIME SETTING (Modify these values)
  int setYear = 2025;
  int setMonth = 4;
  int setDay = 18;
  int setHour = 18;  // 24-hour format
  int setMinute = 28;
  int setSecond = 0;

  // Create and set RTC time
  RtcDateTime newTime = RtcDateTime(
    setYear, setMonth, setDay,
    setHour, setMinute, setSecond
  );
  
  rtc.SetDateTime(newTime);
  
  Serial.println("RTC time set to:");
  printTime();
}

void loop() {
  // Just display time every second
  printTime();
  delay(1000);
}

void printTime() {
  RtcDateTime now = rtc.GetDateTime();
  
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d",
           now.Year(), now.Month(), now.Day(),
           now.Hour(), now.Minute(), now.Second());
  
  Serial.println(buf);
}