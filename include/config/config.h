#ifndef MYCONF_H
#define MYCONF_H
#include<Arduino.h>
#include <IPAddress.h>
#include "soft_version.h"
const uint8_t targetMAC[] = {0x00, 0xe0, 0x4c, 0x38, 0xcd, 0xc2}; // BLACK


#define ENABLE_OTA 1
#define DISABLE_ETHERNET 1
#define ENABLE_WIFI 1
#define MyServ      1
#define OTA_WIFI_ETH 1 // 1 WIFI , O ETHERNET  
//#define WIFI_PASSWD "105719105719" /* "i6q4k3b21r" */ //"97800903"
//#define WIFI_SSID "Globalnet" /* "TT_23C0"     */ //"TP-Link_AP_70CA"
#define TEST_SOFTWARE_VERSION FIRMWARE_VERSION
//192.168.4.35:5000
#define MYSERV_IP (192, 168, 10, 44)
#define MYSERV_PORT 5000
#define SCREE_FLEX_SERV_IP (203, 161, 60, 31)
#define SCREE_FLEX_SERV_PORT 3500

#if (MyServ == 1)
#define SERVER_IP MYSERV_IP
#define SERVER_PORT MYSERV_PORT
#else 
#define SERVER_IP SCREE_FLEX_SERV_IP
#define SERVER_PORT SCREE_FLEX_SERV_PORT
#endif 
#endif


// #include "RtcDateTime.h"
#include "RTC_module.h"

// RTC Module
extern RTC_Module rtc;

extern RtcDateTime beginTime;
extern RtcDateTime endTime;
