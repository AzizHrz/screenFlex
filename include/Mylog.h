#ifndef MY_LOG_H_
#define MY_LOG_H_

#include <Arduino.h>

typedef enum {
    INFO,
    ERROR,
    WARNING, 
    DEBUG
} LogLevel;


void MyLog(LogLevel level, const char* message) ;
void MyLogF(LogLevel level, const char* format, ...);




#endif