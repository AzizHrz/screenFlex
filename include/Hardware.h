/**
 * @file Hardware.h
 * @brief Hardware pin configuration for the project.
 *
 * This file defines all the GPIO pins used for different hardware components,
 * such as buttons, IR sensors, LEDs, and the W5500 Ethernet module.
 * @author Moktar SELLAMI
 * @date 2025-02-16
 */
#ifndef HARDWARE_H
#define HARDWARE_H



/**
 * @def PIN_BTN
 * @brief GPIO pin number for the button.
 */
#define PIN_BTN 12

/**
 * @def IR_SEND_PIN_
 * @brief GPIO pin used for sending IR emitter.
 */
//#define IR_SEND_PIN_ 4
//#define IR_SEND_PIN_ 32
//#define IR_SEND_PIN 32

/**
 * @def IR_RECEIVE_PIN
 * @brief GPIO pin used for receiving IR receiver.
 */
#define IR_RECEIVE_PIN 16 // D15

/**
 * @def TV_STATUS_PIN
 * @brief GPIO pin used to monitor TV status On/Off.
 */
//#define TV_STATUS_PIN 32  
#define TV_STATUS_PIN 39  

/**
 * @def RGB_RED_PIN
 * @brief GPIO pin for controlling the red LED in the RGB module.
 */
#define RGB_RED_PIN 27

/**
 * @def RGB_GREEN_PIN
 * @brief GPIO pin for controlling the green LED in the RGB module.
 */
#define RGB_GREEN_PIN 26

/**
 * @def RGB_BLUE_PIN
 * @brief GPIO pin for controlling the blue LED in the RGB module.
 */
#define RGB_BLUE_PIN 25

/**
 * @def W5500_CS
 * @brief GPIO pin for the Chip Select (CS) of the W5500 Ethernet module.
 */
#define W5500_CS 33

/**
 * @brief W5500 Pinouts: Default SPI pin configuration for ESP32.
 *
 * | Function | GPIO Pin |
 * |----------|---------|
 * | MOSI     | 23      |
 * | MISO     | 19      |
 * | SCLK     | 18      |
 * | CS       | 33      |
 * | VCC      | -       |
 * | GND      | -       |
 */
 
#endif // HARDWARE_H
