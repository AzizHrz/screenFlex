/**
 * @file  soft_version.h
 * @brief This file defines the software versioning and product information for the firmware.
 * 
 * It includes macros for the product name, firmware version, and utilities for stringifying the version number.
 * @author Moktar SELLAMI
 * @date 2025-02-16 
 */

 #ifndef SOFT_VERSION_H
 #define SOFT_VERSION_H
 
 /**
  * @def PRODUCT_NAME
  * @brief The name of the product.
  * 
  * This macro defines the product name used throughout the firmware.
  */
 #define PRODUCT_NAME "Starter"
 
 /**
  * @def STRINGIFY(x)
  * @brief A macro to convert a value to a string.
  * 
  * This macro is used for converting the value passed to it into a string literal.
  * 
  * @param x The value to be stringified.
  */
 #define STRINGIFY(x) #x
 
 /**
  * @def TOSTRING(x)
  * @brief A helper macro that uses STRINGIFY to convert a value into a string.
  * 
  * This macro simplifies the process of stringifying a value.
  * 
  * @param x The value to be stringified.
  */
 #define TOSTRING(x) STRINGIFY(x)
 
 /**
  * @def FIRMWARE_VERSION
  * @brief The firmware version string.
  * 
  * The VERSION is compiler flag, Must be updated by the User. 
  * This macro expands to the stringified version of the firmware version number.
  */
 //#define FIRMWARE_VERSION  TOSTRING(VERSION)
 //#define FIRMWARE_VERSION  TOSTRING(1.1.2) //for firmware1.1.2
 #define FIRMWARE_VERSION  TOSTRING(1.1.1) //for testOTA.cpp
 
 /**
  * @brief Displays the firmware version during compilation.
  * 
  * The `#pragma message` directive outputs the firmware version to the build log during the compilation process.
  */
 #pragma message("Firmware version: " FIRMWARE_VERSION)
 
 #endif 
 