#ifndef SOFT_VERSION_H
#define  SOFT_VERSION_H

#define PRODUCT_NAME "STARTER"


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FIRMWARE_VERSION  TOSTRING(VERSION)

#pragma message("Firmware version: " FIRMWARE_VERSION)


#endif 