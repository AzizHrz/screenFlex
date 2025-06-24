#include "ButtonModule.h"
#include <Arduino.h>

extern volatile bool Wake_PC_STATE_finished;
static unsigned long pressStart;
static bool longPressed_5s ;
static bool longPressed_10s;
ButtonModule::ButtonModule(uint8_t _pinbtn){
    pinbtn = _pinbtn;
    pressStart = 0;
    longPressed_5s = false;
    longPressed_10s = false;
}

void ButtonModule::InitButton()
{
    pinMode(pinbtn, INPUT_PULLUP); // Assuming active LOW button
}
void ButtonModule::HandleButton_5s()
{
    static bool btn_pressed = false;
    // static bool long_pressed_d11etected = false;
    static unsigned long period = 0;

    bool btn_state = !(digitalRead(pinbtn)); //! // Assuming active LOW button
    /* Serial.println(btn_state); */
    if (btn_state)
    { // Button is pressed
        if (!btn_pressed)
        { // First press detected
            btn_pressed = true;
            period = millis(); // Record the time of the press
        }
        if (millis() - period >= 5000 && !longPressed_5s)
        { // Long press detected
            Serial.println("btn pressed for more than 5s");
            longPressed_5s = true;
            btn_pressed = false; // Reset for new detection
          
        }
        // if ( millis() - period >= 10000){
        //     Serial.println("Reseting ESP32 NOW ..... ");
        //     longPressed_10s = true;
        //     btn_pressed = false; // Reset for new detection

        //     return ;
        // }
    }
    else{                        
        btn_pressed = false; // Reset press state
        longPressed_5s = false; // Allow new long press detection
        period = 0;             // Reset the timer
        // program_status = INIT;
    }

    return; // No long press detected
}

void ButtonModule::TaskResetESP32(void *prameter)
{
    // TODO 
    // static bool btn_pressed = false;
    // // static bool long_pressed_d11etected = false;
    // static unsigned long period = 0;
    // while (1)
    // {

    //     // Serial.println("task running ....");
    //     bool btn_state = digitalRead(pinbtn); //! // Assuming active LOW button
    //     /* Serial.println(btn_state); */
    //     if (btn_state)
    //     { // Button is pressed
    //         if (!btn_pressed)
    //         { // First press detected
    //             btn_pressed = true;
    //             period = millis(); // Record the time of the press
    //         }

    //         if (millis() - period >= 10000)
    //         {
    //             Serial.println("Reseting ESP32 NOW ..... ");
    //             btn_pressed = false; // Reset for new detection

    //             ESP.restart();
    //         }
    //     }
    //     else
    //     {                        // Button is released
    //         btn_pressed = false; // Reset press state

    //         period = 0; // Reset the timer
    //     }
    //     vTaskDelay(50 / portTICK_PERIOD_MS);
    // }
}

bool ButtonModule::isLongPressed_5s()
{
    return longPressed_5s;
}
void ButtonModule::runResetESP32Task(){ // TODO
//       xTaskCreate(TaskResetESP32, // Function to run as a thread
//               "Reset ESP ", // Name of the task
//               1024,       // Stack size in words
//               NULL,       // Task input parameter
//               2,          // Task priority
//               NULL        // Task handle (optional)
//   );
}
