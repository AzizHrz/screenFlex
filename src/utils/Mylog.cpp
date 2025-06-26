#include <Arduino.h>
#include "utils/Mylog.h"

#define LOG_COLOR_RESET  "\033[0m"
#define LOG_COLOR_BLUE   "\033[34m"  // Blue for INFO
#define LOG_COLOR_RED    "\033[31m"  // Red for ERROR
#define LOG_COLOR_YELLOW "\033[33m"  // Yellow for WARNING
#define LOG_COLOR_GREEN  "\033[32m"  // Green for DEBUG

static const char* getColor(LogLevel level) {
    switch (level) {
        case INFO:    return LOG_COLOR_BLUE;
        case ERROR:   return LOG_COLOR_RED;
        case WARNING: return LOG_COLOR_YELLOW;
        case DEBUG:   return LOG_COLOR_GREEN;
        default:      return LOG_COLOR_RESET;
    }
}

static void printLogLevel(LogLevel level) {
    switch (level) {
        case INFO:    Serial.print("[INFO] "); break;
        case ERROR:   Serial.print("[ERROR] "); break;
        case WARNING: Serial.print("[WARNING] "); break;
        case DEBUG:   Serial.print("[DEBUG] "); break;
    }
}

void MyLog(LogLevel level, const char* message) {
    const char* color = getColor(level);
    Serial.print(color);
    printLogLevel(level);
    Serial.print(message);
    Serial.println(LOG_COLOR_RESET);
}

void MyLogF(LogLevel level, const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    const char* color = getColor(level);
    Serial.print(color);
    printLogLevel(level);
    Serial.print(buffer);
    Serial.println(LOG_COLOR_RESET);
}