// i will create this newwww module
// ScheduleUtils.h
#ifndef SCHEDULE_UTILS_H
#define SCHEDULE_UTILS_H

#include <Arduino.h>
#include "dataModel.h"

bool checkSchedule(const ScheduleData& schedule);
bool parseScheduleResponse(const String& payload);
bool isDaySelected(String day);

#endif // SCHEDULE_UTILS_H
