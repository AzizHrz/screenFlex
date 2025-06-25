// i will create this newwww module
// ScheduleUtils.h
#ifndef SCHEDULE_UTILS_H
#define SCHEDULE_UTILS_H

#include <Arduino.h>
#include "dataModel.h"
#include <ArduinoJson.h>
#include "modules/RTC_module.h"
#include "config.h"
#include "Mylog.h"
#include "dataModel.h"
#include "LittleFS.h"

// 1
bool checkSchedule(const ScheduleData& schedule);
bool parseScheduleResponse(const String& payload);
bool isDaySelected(String day);

// 2
bool saveSchedule(const ScheduleData& schedule);
bool loadSchedule(ScheduleData& schedule);
bool updateScheduleFromServer();

#endif // SCHEDULE_UTILS_H
