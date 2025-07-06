/**
 * @file RGBModule.h
 * @brief This file defines the RGBModule class for controlling an RGB LED.
 * 
 * The RGBModule class allows setting colors, blinking status, and managing RGB LED tasks.
 * @author Moktar SELLAMI
 * @date 2025-02-16
 */

 #ifndef RGB_LED_H
 #define RGB_LED_H
 
 #include <Arduino.h>
 
 /**
  * @enum LED_STATUS_COLOR
  * @brief Enumeration for the different LED status colors.
  * 
  * This enum represents various states for controlling LED colors.
  */
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
 
 /**
  * @class RGBModule
  * @brief Class for controlling an RGB LED.
  * 
  * This class provides functionality to initialize, set colors, and control
  * the blinking of an RGB LED using the specified pin numbers for red, green, and blue.
  */
 class RGBModule
 {
 public:
     /**
      * @brief Constructor for RGBModule.
      * 
      * @param rpin Pin number for the red LED.
      * @param gpin Pin number for the green LED.
      * @param bpin Pin number for the blue LED.
      */
     RGBModule(int rpin, int gpin, int bpin);
 
     /**
      * @brief Initializes the RGB LED pins.
      */
     void RGBInit();
 
     /**
      * @brief Sets the RGB LED to a specific color.
      * 
      * @param redValue Value for the red color (0-255).
      * @param greenValue Value for the green color (0-255).
      * @param blueValue Value for the blue color (0-255).
      */
     void setColor(int redValue, int greenValue, int blueValue);
 
     /**
      * @brief Starts the RGB task for blinking.
      */
     void runRGBTask();
 
     /**
      * @brief Stops the RGB blinking task.
      */
     void stopRGBTask();
 
     /**
      * @brief Kills the RGB task.
      */
     void killRGBTask();
 
     /**
      * @brief Sets the blinking status of the LED.
      * 
      * @param s The color status to be used for blinking.
      */
     void setBlinkingStatus(LED_STATUS_COLOR s);
 
 private:
     /**
      * @brief Toggles the LED state (used in a task).
      * 
      * This is a static function used by FreeRTOS to toggle the LED state
      * periodically in a task.
      * 
      * @param parameter The task's parameter, typically an instance of RGBModule.
      */
     static void toggleLEDTask(void *parameter);
 
 private:
     TaskHandle_t RGBTaskHandle; /**< Handle for the RGB task. */
     volatile LED_STATUS_COLOR BlinkingStatus; /**< Current blinking status. */
     uint8_t rpin; /**< Pin for the red LED. */
     uint8_t gpin; /**< Pin for the green LED. */
     uint8_t bpin; /**< Pin for the blue LED. */
 };
 
 #endif
 