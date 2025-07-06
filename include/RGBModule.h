#ifndef RGB_LED_H
#define RGB_LED_H
#include <Arduino.h>

typedef enum
 {
     GREEN_COLOR_STATUS = 0, /**< LED is green. */
     BLUE_COLOR_STATUS,      /**< LED is blue. */
     RED_COLOR_STATUS,       /**< LED is red. */
     NO_COLOR_STATUS,        /**< LED is off. */
     YELLOW_COLOR_STATUS,    /**< LED is yellow. */
     LIGHT_BLUE_COLOR_STATUS,/**< LED is light blue. */
     BRIGHT_ORANGE_COLOR_STATUS, /**< LED is bright orange. */
     WHITE_COLOR_STATUS,
     VIOLET_COLOR_STATUS
 
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
