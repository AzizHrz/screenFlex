/**
 * @file IR_Module.h
 * @brief Infrared (IR) module for handling IR communication.
 *
 * This module provides functionality to receive, send, store, and process IR signals.
 * It utilizes the IRremote library to interact with IR hardware components.
 *
 * @author Moktar SELLAMI
 * @date 2025-02-16
 */

 #pragma once

 #ifndef MY_IR_MODULE_H_
 #define MY_IR_MODULE_H_
 
 #include <IRremoteInt.h>
 #include <Arduino.h>
 #include <LittleFS.h>
 #include <ArduinoJson.h>
 #include "Mylog.h"
 
 /// File path for storing settings.
 #define f "/settings.json"
 
 /**
  * @enum Files
  * @brief Defines file types used in the system.
  */
 typedef enum {
     settings,  ///< Standard settings file.
     jsettings  ///< JSON settings file.
 } Files;
 
 /**
  * @struct MyIRData
  * @brief Structure to store received IR data.
  */
 typedef struct {
     bool data_stored;                  ///< Indicates if data is stored.
     decode_type_t protocol;             ///< Protocol type.
     IRRawDataType DecodedRawDataArray[RAW_DATA_ARRAY_SIZE]; ///< Raw data array.
     DistanceWidthTimingInfoStruct timingInfo; ///< Pulse distance timing info.
     uint8_t numberOfBits;               ///< Number of bits in the received command.
     uint16_t IR_Adress;                 ///< Address of the IR signal.
     uint16_t IR_Command;                ///< Command of the IR signal.
 } MyIRData;
 
 /**
  * @class IRHandler
  * @brief Class for handling IR communication.
  */
 class IRHandler {
 public:
     /**
      * @brief Constructor for IRHandler.
      * @param receivePin IR receive pin.
      * @param _sendPin IR send pin.
      * @param tvpin TV control pin.
      */
     IRHandler(uint8_t receivePin, uint8_t _sendPin, uint8_t tvpin);
 
     /** @brief Initializes the IR module. */
     void begin();
 
     /** @brief Initializes LittleFS. */
     void init_littleFS();
 
     /** @brief Dumps stored IR data that are store in a file. to the console */
     void dump_data_stored(const char* filename);
 
     /**
      * @brief Identifies the protocol of received IR data.
      * @param irData Reference to MyIRData structure.
      * @return Detected protocol type.
      */
     decode_type_t identifyProtocol(MyIRData& irData);
 
     /**
      * @brief Sets received IR data.
      * @param irData Reference to MyIRData structure.
      */
     void setRecievedData(MyIRData& irData);
 
     /**
      * @brief Receives IR data and optionally saves it.
      * @param saveData Boolean flag to store data.
      * @return True if data is successfully received, false otherwise.
      */
     bool receiveIR(bool saveData);
 
     /** @brief Sends stored IR data. */
     void sendIR();
 
     /**
      * @brief Plays back stored IR data.
      * @param irData IR data to be played back.
      */
     void playBackData(const MyIRData& irData);
 
     /**
      * @brief Checks TV status after a certain delay.
      * @param delay Time delay before checking status.
      * @return TV status (0 or 1).
      */
     uint8_t getTVStatus(uint16_t delay);
 
     /**
      * @brief Stores received IR data.
      * @param irData Reference to MyIRData structure.
      */
     void storeData(MyIRData& irData);
 
     /**
      * @brief Checks if IR data is available in a file.
      * @param filename File to check.
      * @return True if data is available, false otherwise.
      */
     bool IsIRDataavailable(const char* filename);
 
     /**
      * @brief Reads stored IR data from a JSON file and saves it in data.
      * @param filename File to read.
      * @param data Reference to MyIRData structure to populate.
      * @return True if successful, false otherwise.
      */
     bool readMyIRDataJSON(const char* filename, MyIRData& data);
 
     /**
      * @brief Stores IR data into a JSON file.
      * @param filename File to store data.
      * @param data Data to be stored.
      */
     void storeMyIRDataJSON(const char* filename, const MyIRData& data);
 
     /**
      * @brief Dumps IR data stored a JSON file to the console.
      * @param filename File to dump data.
      * @return True if successful, false otherwise.
      */
     bool DumpMyIRDataJSON(const char* filename);
 
     /**
      * @brief Prints stored IR data to the console.
      * @param data IR data to print.
      */
     void PrintMyIRData(MyIRData& data);
 
     /**
      * @brief Clears the content of a file.
      * @param filename File to clear.
      */
     void clear_file(const char* filename);
 
 private:
     uint8_t receivePin; ///< IR receive pin.
     uint8_t _sendPin;   ///< IR send pin.
     uint8_t _tvpin;     ///< TV control pin.
     IRrecv MyIrReceiver; ///< IR receiver object.
     IRsend MyIrSender;   ///< IR sender object.
     MyIRData receivedData; ///< Stores the last received IR data.
 };
 
 #endif // MY_IR_MODULE_H_