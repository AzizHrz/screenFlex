#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <Arduino.h>
// Schedule Data Structures
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


#endif // SCHEDULE_H