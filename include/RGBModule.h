#ifndef RGB_LED_H
#define RGB_LED_H
#include <Arduino.h>

typedef enum
{
    GREEN_COLOR_STATUS = 0,
    BLUE_COLOR_STATUS,
    RED_COLOR_STATUS,
    NO_COLOR_STATUS,
    YELLOW_COLOR_STATUS,
    LIGHT_BLUE_COLOR_STATUS,
    // GREEN_COLOR_STATUS,
    // GREEN_COLOR_STATUS,
    // GREEN_COLOR_STATUS,
} LED_STATUS_COLOR;

class RGBModule
{

public:
    RGBModule(int rpin, int gpin, int bpin);
    void RGBInit();
    void setColor(int redValue, int greenValue, int blueValue);
    void runRGBTask();
    void stopRGBTask();
    void killRGBTask();

    void setBlinkingStatus(LED_STATUS_COLOR s);
    
    static void toggleLEDTask(void *parameter);

private:
    TaskHandle_t RGBTaskHandle;
    volatile LED_STATUS_COLOR BlinkingStatus;
    uint8_t rpin;
    uint8_t gpin;
    uint8_t bpin;
};

#endif
