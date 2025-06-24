// newww file
#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <Arduino.h>

struct WiFiConfig {
    String ssid;
    String password;
};

struct ModeData {
    bool useSchedule;
};

struct ServerCommand {
    bool state;
};

struct ScheduleData {
    bool active;
    String days[7];
    int daysCount;
    String startTime;
    String endTime;
    String timezone;
};

enum TVState {
    TV_UNKNOWN,
    TV_OFF,
    TV_ON,
    TV_TRANSITIONING
};

struct PersistentData {
    ModeData mode;
    ScheduleData schedule;
    TVState lastCommandedState;
    unsigned long lastUpdate;
};

#endif // DATA_MODEL_H
