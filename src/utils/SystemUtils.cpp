#include "utils/SystemUtils.h"
#include "utils/Mylog.h"

extern int TV_STATUS_PIN;

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
