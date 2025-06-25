#ifndef PERSISTENCE_H
#define PERSISTENCE_H


// 1 
bool savePersistentData();
bool loadPersistentData();
/////////////////////////

// 2
#include "dataModel.h"

bool saveWiFiConfig(const WiFiConfig& config);
bool loadWiFiConfig(WiFiConfig& config);

bool saveCurrentVersion(const String& version);
String loadCurrentVersion();

bool saveMode(const ModeData& mode);
bool loadMode(ModeData& mode);

#endif // PERSISTENCE_H
