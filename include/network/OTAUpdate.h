// newww file
// OTAUpdate.h


#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>

bool checkForUpdates();
bool performOTAUpdate(String firmwareUrl);
void setupOTA();

#endif // OTA_UPDATE_H

