#ifndef BUTTON_H 
#define BUTTON_H 
#include<Arduino.h>


class ButtonModule{ 
public: 
ButtonModule(uint8_t _pinbtn);  
void InitButton(); 
void HandleButton_5s() ;
bool isLongPressed_5s();

void runResetESP32Task(); 
void stopResetESP32Task();
void killResetESP32Task();
private: 
static void TaskResetESP32(void* prameter);
private:

uint8_t pinbtn; 
TaskHandle_t handler;

};
#endif 