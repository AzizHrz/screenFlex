#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <Arduino.h>

uint8_t checkTVStatus();
bool is_newer_version(const String& current, const String& available);

#endif // SYSTEM_UTILS_H
