/**
 * @file conf.h
 * @brief Project configuration header file.
 *
 * This file contains project configurations used at build time.
 * - Enable/Disable OTA
 * - Enable/Disable WiFi
 * - Enable/Disable Ethernet
 * - Select test server or production server
 * - Set WiFi SSID And PAsswords 
 * @author Moktar SELLAMI
 * @date 2025-02-16 
 */
#ifndef MYCONF_H
#define MYCONF_H



#include <Arduino.h>      ///< Arduino library
#include <IPAddress.h>    ///< IP address library
#include "soft_version.h" ///< Software version module

/**
 * @brief Target MAC address for Wake-on-LAN.
 */
const uint8_t targetMAC[] = {0x00, 0xe0, 0x4c, 0x38, 0xcd, 0xc2};

/** 
 * @def ENABLE_OTA
 * @brief Flag to enable or disable Over-the-Air (OTA) updates.
 * @note 1 to enable, 0 to disable.
 */
#define ENABLE_OTA 0

/** 
 * @def DISABLE_ETHERNET
 * @brief Flag to enable or disable Ethernet.
 * @note 1 to disable, 0 to enable.
 */
#define DISABLE_ETHERNET 0
#define ENABLE_ETHERNET 0

/** 
 * @def ENABLE_WIFI
 * @brief Flag to enable or disable WiFi.
 * @note 1 to enable, 0 to disable.
 */
//#define ENABLE_WIFI 0
#define ENABLE_WIFI 1
/** 
 * @def MyServ
 * @brief Flag to select the server.
 * @note 1 = Test Server, 0 = Production Server.
 */
#define MyServ 1

/** 
 * @def OTA_WIFI_ETH
 * @brief Flag to select WiFi or Ethernet for OTA.
 * @note 1 = WiFi, 0 = Ethernet.
 */
#define OTA_WIFI_ETH 1

/** 
 * @def WIFI_PASSWD
 * @brief WiFi password.
 */
#define WIFI_PASSWD "i6q4k3b21r"

/** 
 * @def WIFI_SSID
 * @brief WiFi SSID.
 */
#define WIFI_SSID "TT_23C0"

/** 
 * @def TEST_SOFTWARE_VERSION
 * @brief Firmware version used for OTA update comparisons.
 */
#define TEST_SOFTWARE_VERSION FIRMWARE_VERSION

/** 
 * @def MYSERV_IP
 * @brief IP address of the test server.
 */
//172.20.10.2:5000
// #define MYSERV_IP (192, 168, 1, 14) 
// #define MYSERV_IP (172, 20, 10, 2) 
//92.168.1.173:5000
// my server ip
//#define MYSERV_IP (192, 168, 1, 173) 
#define MYSERV_IP (172, 16, 137, 146) 

/** 
 * @def MYSERV_PORT
 * @brief Port number of the test server.
 */
#define MYSERV_PORT 5000

/** 
 * @def SCREE_FLEX_SERV_IP
 * @brief IP address of the production server.
 */
#define SCREE_FLEX_SERV_IP (203, 161, 60, 31)

/** 
 * @def SCREE_FLEX_SERV_PORT
 * @brief Port number of the production server.
 */
#define SCREE_FLEX_SERV_PORT 3500

/** 
 * @brief Selects server IP and port based on `MyServ`.
 * 
 * If `MyServ == 1`, the server IP and port are assigned from `MYSERV_IP`
 * and `MYSERV_PORT`, respectively. Otherwise, they are assigned from
 * `SCREE_FLEX_SERV_IP` and `SCREE_FLEX_SERV_PORT`.
 */
#if (MyServ == 1)
    #define SERVER_IP MYSERV_IP     /**< Server IP when MyServ == 1 */
    #define SERVER_PORT MYSERV_PORT /**< Server Port when MyServ == 1 */
#else
    #define SERVER_IP SCREE_FLEX_SERV_IP     /**< Default Server IP */
    #define SERVER_PORT SCREE_FLEX_SERV_PORT /**< Default Server Port */
#endif

#endif//  MYCONF_H
