#ifndef ESP32_AP_STA_H
#define ESP32_AP_STA_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>



void initWiFiAPSTA();     // Setup AP+STA and WebServer
void handleRoot();        // HTML form
void handleConnect();     // Handle WiFi connection
void handleStatus();      // Display connection result
void handleTokenUser() ; // Display token and user
void handleWiFiServer();
void processWiFiConnection() ;
void loopWifiServer() ;

#endif
