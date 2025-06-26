#ifndef MY_IR_MODULE_H_
#define MY_IR_MODULE_H_



#include <Arduino.h>
#include <IRremote.hpp>
#include <IRProtocol.h>
#include "IR_PIN_DEF.h" 
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "utils/Mylog.h"

#define DECODE_NEC          // Includes Apple and Onkyo. To enable all protocols, just comment/disable this line.
#define _MICROS_PER_TICK 50
#define SEND_PWM_BY_TIMER // We do not have pin restrictions for this CPU's, so lets use the hardware PWM for send carrier signal generation

#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif


#define f "/settings.json"

 // Define macros for input and output pin etc.
// #include "IR_PIN_DEF.h"
typedef enum {
 settings,
 jsettings,
} Files;


String file_names[] = {"/settings.txt","/settings.json"};

typedef struct {
    bool data_stored; 
    decode_type_t protocol;                 // Protocol type
  IRRawDataType DecodedRawDataArray[RAW_DATA_ARRAY_SIZE] ;
    DistanceWidthTimingInfoStruct timingInfo;  // Used for pulse distance timing
    uint8_t numberOfBits;
              // Number of bits in the received command
    // uint8_t rawData[RAW_BUFFER_LENGTH];  // Raw IR data for Pulse Distance
 
    uint16_t IR_Adress; 
    uint16_t IR_Command;

} MyIRData;


class IRHandler {
public:
    void init_littleFS(); 
    void dump_data_stored(const char* filename);
    IRHandler(uint8_t receivePin, uint8_t _sendPin, uint8_t tvpin){
        
        this->receivePin = receivePin;
        this->_sendPin = _sendPin; // Use the member variable with underscore
        this->_tvpin = tvpin;
        this->receivedData.data_stored = false; // Initialize data_stored to false
    }
    void begin();
    decode_type_t identifyProtocol(MyIRData& irData);
    void setRecievedData(MyIRData& irData);
    
    bool receiveIR(bool saveData);
    void sendIR();
    void playBackData(const MyIRData& irData);
    uint8_t getTVStatus(uint16_t delay);// this funtion performs a liitle delay and returns the TV status  

    void storeData(MyIRData& irData);
    bool IsIRDataavailable(const char* filename); 
    bool readMyIRDataJSON(const char* filename, MyIRData& data);
    void storeMyIRDataJSON(const char* filename, const MyIRData& data);
    bool DumpMyIRDataJSON(const char* filename) ;
    void PrintMyIRData( MyIRData& data);
    void clear_file(const char* filename);

    
private:
    uint8_t receivePin;
    uint8_t _sendPin; // Member variable with underscore
    uint8_t _tvpin;

    MyIRData receivedData; // Store the last received IR data
};
void IRHandler::setRecievedData(MyIRData& irData){ 
    receivedData=irData; 
    
}
uint8_t IRHandler::getTVStatus(uint16_t checkDuration) {
    // Disable IR receiver to prevent interference
    IrReceiver.stop();
    
    // Configure pin properly (in case other code modified it)
    pinMode(_tvpin, INPUT_PULLUP);
    
    const uint8_t requiredSamples = 3;
    uint8_t consistentReadings = 0;
    uint8_t lastState = digitalRead(_tvpin);
    
    uint32_t startTime = millis();
    while (millis() - startTime < checkDuration) {
        uint8_t currentState = digitalRead(_tvpin);
        
        if (currentState == lastState) {
            consistentReadings++;
            if (consistentReadings >= requiredSamples) {
                break; // Stable state detected
            }
        } else {
            lastState = currentState;
            consistentReadings = 1;
        }
        
        delay(20); // Short delay between samples
    }
    
    // Re-enable IR receiver
    IrReceiver.start();
    
    return (lastState == LOW) ? 1 : 0; // LOW = ON (1), HIGH = OFF (0)


/*
    const uint8_t requiredConsistentReadings = 3; // Number of stable readings
    const uint16_t sampleInterval = 50;           // ms between samples

    uint8_t lastStableState = !digitalRead(_tvpin); // Inverted: LOW = ON -> 1
    uint8_t consistentCount = 1;

    for (uint16_t elapsed = sampleInterval; elapsed < checkDuration; elapsed += sampleInterval) {
        uint8_t currentState = !digitalRead(_tvpin);

        if (currentState == lastStableState) {
            consistentCount++;
            if (consistentCount >= requiredConsistentReadings) {
                return currentState; // Return when readings are stable
            }
        } else {
            lastStableState = currentState;
            consistentCount = 1; // Reset count with new state
        }

        delay(sampleInterval);
    }

    // If no stable state detected in time, return the last observed state
    return lastStableState;
#########################################################
    uint8_t getTVStatus(uint16_t timeout) {
  // Add a small delay to avoid flooding the Serial Monitor
  delay(checkDuration); // Adjust as needed
  if (_tvpin == LOW) {
    return 1; // TV is ON
  } else {
   return 0; // TV is OFF
  }

  
}*/
    
    }




void IRHandler::PrintMyIRData( MyIRData& data){ 
  Serial.println("------ MyIRData ------");
    Serial.print("Data Stored: ");
    Serial.println(data.data_stored ? "Yes" : "No");
    
    Serial.print("Protocol: ");
    Serial.println(static_cast<int>(data.protocol));
    
    Serial.print("Number of Bits: ");
    Serial.println(data.numberOfBits);
    
    Serial.print("IR Address: ");
    Serial.println(data.IR_Adress);
    
    Serial.print("IR Command: ");
    Serial.println(data.IR_Command);
    
    Serial.println("Decoded Raw Data Array:");
    for (int i = 0; i < RAW_DATA_ARRAY_SIZE; i++) {
        Serial.print("  [");
        Serial.print(i);
        Serial.print("]: ");
        Serial.println(data.DecodedRawDataArray[i]);
    }
    
    Serial.println("Distance Width Timing Info:");
    Serial.print("  Header Mark Micros: ");
    Serial.println(data.timingInfo.HeaderMarkMicros);
    
    Serial.print("  Header Space Micros: ");
    Serial.println(data.timingInfo.HeaderSpaceMicros);
    
    Serial.print("  One Mark Micros: ");
    Serial.println(data.timingInfo.OneMarkMicros);
    
    Serial.print("  One Space Micros: ");
    Serial.println(data.timingInfo.OneSpaceMicros);
    
    Serial.print("  Zero Mark Micros: ");
    Serial.println(data.timingInfo.ZeroMarkMicros);
    
    Serial.print("  Zero Space Micros: ");
    Serial.println(data.timingInfo.ZeroSpaceMicros);
    
    Serial.println("----------------------");
}
void IRHandler::dump_data_stored(const char* filename){ 

    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
        if ( !file.available()){ 
        Serial.println(" no available content Content:");

        }
        Serial.println("Content:");
        while (file.available()) {
            Serial.write(file.read());
            Serial.flush();
        }
        Serial.println("\n---------------------");
    file.close();

}

void IRHandler::init_littleFS(){ 

     if (!LittleFS.begin()) {
        Serial.println("[ERROR] Failed to initialize LittleFS");
        return ;
    } else { 
        Serial.println("[INFO] LittleFS initialized successfully");

    }
}



// Setup: Begin IRReceiver and IRSender
void IRHandler::begin() {
  Serial.begin(115200);
    Serial.print("Initializing IR on receive pin: ");
    Serial.println(receivePin);
    
    IrReceiver.begin(receivePin, ENABLE_LED_FEEDBACK);
    IrSender.begin(_sendPin);
    
    Serial.println("IR initialized");
  
  if (!LittleFS.begin()) {
    MyLog(ERROR, "Failed to initialize LittleFS");
    return;
  }

  // Try to load saved IR data
  if (!readMyIRDataJSON(f, receivedData)) {
    MyLog(INFO, "No saved IR data found");
    receivedData.data_stored = false;
  } else {
    MyLog(INFO, "Loaded saved IR data");
  }
}

// Receive IR signal and identify protocol
bool IRHandler::receiveIR(bool saveData) {
    IrReceiver.start();
    while (true){ 
        if (IrReceiver.decode()) {
          IrReceiver.printIRResultShort(&Serial); 
          Serial.println(); 
          IrReceiver.printIRResultRawFormatted(&Serial); 
          Serial.println(); 
            MyIRData irData;
            irData.numberOfBits = IrReceiver.decodedIRData.numberOfBits;
            irData.protocol = identifyProtocol(irData);
            // if ( )
            // Store the raw data based on the protocol
            if (irData.protocol == PULSE_DISTANCE) {
                memcpy(irData.DecodedRawDataArray, IrReceiver.decodedIRData.decodedRawDataArray, sizeof(irData.DecodedRawDataArray));
                irData.timingInfo = IrReceiver.decodedIRData.DistanceWidthTimingInfo;
                // irData.data_Read= true; 
            } else {
                irData.IR_Adress = IrReceiver.decodedIRData.address; 
                irData.IR_Command = IrReceiver.decodedIRData.command;
                // irData.data_Read= true; 
                // Handle Samsung or LG protocol
                // Add specific code to handle Samsung and LG data formats
            }
            if ( saveData){ 
            storeData(irData);
            MyLog(INFO,"DATA Stored ...");
            }
            IrReceiver.resume(); // Ready to receive next data
            IrReceiver.stop();
            return true;
        } delay(100);

}
MyLog(ERROR, "IR receive timeout");
    return false;
}

// Identify the protocol type of the received IR signal
decode_type_t IRHandler::identifyProtocol(MyIRData& irData) {
    // Here you can compare against known protocols (e.g., Pulse Distance, Samsung, LG)
    // Serial.println(IrReceiver.decodedIRData.protocol)
    if (IrReceiver.decodedIRData.protocol == PULSE_DISTANCE) {
        return PULSE_DISTANCE;
    } else if (IrReceiver.decodedIRData.protocol == SAMSUNG) {
        return SAMSUNG;
    } else if (IrReceiver.decodedIRData.protocol == LG) {
        return LG;
    }else if (IrReceiver.decodedIRData.protocol == NEC) {
        return NEC;
    } else {
        return UNKNOWN;
    }
}

// Store received IR data in memory
void IRHandler::storeData(MyIRData& irData) {
    // Store IR data in receivedData (you can expand this to use a more advanced storage mechanism)
    receivedData = irData;
    Serial.println("Stored new IR data.");
    receivedData.data_stored=true; 
    storeMyIRDataJSON(f,receivedData);
    

}

// Send the stored IR data based on protocol
void IRHandler::sendIR() {
        IrReceiver.stop();
        Serial.println("Seding data "); 


    if (receivedData.protocol == PULSE_DISTANCE) {
        Serial.println("SENDING PULSE DISTANCE PROTOCOLE ");
        // IrSender.sendPulseDistanceWidthFromArray(38, &sDistanceWidthTimingInfo, &sDecodedRawDataArray[0], sNumberOfBits,PROTOCOL_IS_LSB_FIRST, 100, 0);
        IrSender.sendPulseDistanceWidthFromArray(38, &receivedData.timingInfo, &receivedData.DecodedRawDataArray[0], receivedData.numberOfBits, PROTOCOL_IS_LSB_FIRST,100,0);
    }else if (receivedData.protocol == NEC ){ 
        IrSender.sendNEC(receivedData.IR_Adress,receivedData.IR_Command,38);
        Serial.println("SENDING NEC PROTOCOLE ");
    } 
    else if (receivedData.protocol == SAMSUNG) {
        Serial.println("SENDING SAMSUNG PROTOCOLE ");
        IrSender.sendSamsung(receivedData.IR_Adress,receivedData.IR_Command,19);
        // IrSender.sendRaw(IrReceiver.decodedIRData.rawDataPtr->rawbuf,IrReceiver.decodedIRData.rawlen,38);
        // IrSender.write(IrReceiver.decodedIRData);
        // Send Samsung IR command (implement logic for Samsung)
    } else if (receivedData.protocol == LG) {
        Serial.println("SENDING PULSE LG ");
        IrSender.sendLG(receivedData.IR_Adress,receivedData.IR_Command,38);

        // Send LG IR command (implement logic for LG)
    } else {
        Serial.println("Unknown protocol, cannot send, Protocole not supported .");
        // Serial.printf()
        IrReceiver.printIRResultShort(&Serial);

    }
    IrReceiver.start();
}

// Play back stored IR data (e.g., when button is pressed)
void IRHandler::playBackData(const MyIRData& irData) {
    sendIR();
}
void IRHandler::clear_file(const char* filename) {
    MyLog(INFO, "Clearing IR data file...");
    
    // Create empty IR data structure
    MyIRData blankData = {
        false,           // data_stored
        UNKNOWN,         // protocol
        {0},             // DecodedRawDataArray
        {0,0,0,0,0,0},   // timingInfo
        0,              // numberOfBits
        0,              // IR_Adress
        0               // IR_Command
    };

    // Store the blank data
    storeMyIRDataJSON(filename, blankData);
    
    // Verify the file was cleared
    if (LittleFS.exists(filename)) {
        File file = LittleFS.open(filename, "r");
        if (file && file.size() > 0) {
            MyLog(INFO, "IR data file cleared successfully");
        } else {
            MyLog(ERROR, "Failed to clear IR data file");
        }
        if (file) file.close();
    } else {
        MyLog(ERROR, "IR data file doesn't exist after clear attempt");
    }
}
void IRHandler::storeMyIRDataJSON(const char* filename, const MyIRData& data) {


    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Create a JSON object
    StaticJsonDocument<512> doc;
    doc["stored"] = data.data_stored;
    doc["protocol"] = (int)data.protocol;
    doc["numberOfBits"] = data.numberOfBits;
    doc["IR_Adress"] = (uint32_t)data.IR_Adress;
    doc["IR_Command"] = (uint32_t)data.IR_Command;

    // Add raw_data as an array

    for (int i = 0 ; i<RAW_DATA_ARRAY_SIZE ; i++){ 
        Serial.println(data.DecodedRawDataArray[i]);
        
    }
    JsonArray array = doc.createNestedArray("raw_data");

    for (uint64_t number : data.DecodedRawDataArray) { // Assuming DecodedRawDataArray is iterable

        array.add(number);
    }

    // Add DistanceWidthTimingInfoStruct as a nested object
    JsonObject timingInfoJson = doc.createNestedObject("timing_info");
    timingInfoJson["HeaderMarkMicros"] = data.timingInfo.HeaderMarkMicros;
    timingInfoJson["HeaderSpaceMicros"] = data.timingInfo.HeaderSpaceMicros;
    timingInfoJson["OneMarkMicros"] =     data.timingInfo.OneMarkMicros;
    timingInfoJson["OneSpaceMicros"] =    data.timingInfo.OneSpaceMicros;
    timingInfoJson["ZeroMarkMicros"] =    data.timingInfo.ZeroMarkMicros;
    timingInfoJson["ZeroSpaceMicros"] =   data.timingInfo.ZeroSpaceMicros;

    // Serialize the JSON object
    serializeJsonPretty(doc, Serial);

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write JSON data");
    } else {
        Serial.println("JSON data stored successfully.");
    }

    file.close();
}

bool IRHandler::IsIRDataavailable(const char* filename){
         File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }


    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Failed to parse JSON data: ");
        Serial.println(error.c_str());
        file.close();
        return false;
    }

    file.close();
    // Populate the MyIRData struct
    bool x = doc["stored"].as<bool>();
    if ( x){ 
        return true ; 
    }else{ 
        return false ; 

    }

    }
// Read JSON data and deserialize it to MyIRData
bool IRHandler::DumpMyIRDataJSON(const char* filename) {


    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }


    String s =""; 
     Serial.println("- read from file:");
  
    // Deserialize the JSON object
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Failed to parse JSON data: ");
        Serial.println(error.c_str());
        file.close();
        return false;
    }
    serializeJsonPretty(doc, Serial);

    // Populate the structur

    file.close();
    Serial.println("JSON data read successfully.");
    return true;
}


bool IRHandler::readMyIRDataJSON(const char* filename, MyIRData& data) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Failed to parse JSON data: ");
        Serial.println(error.c_str());
        file.close();
        return false;
    }

    // Populate the MyIRData struct
    data.data_stored = doc["stored"].as<bool>();
    if ( !data.data_stored ){
        Serial.print("NO  JSON data stored ");
        file.close();
        return false; 
     }
    data.protocol = static_cast<decode_type_t>(doc["protocol"].as<int>());
    data.numberOfBits = doc["numberOfBits"].as<uint8_t>();
    data.IR_Adress = doc["IR_Adress"].as<uint16_t>();
    data.IR_Command = doc["IR_Command"].as<uint16_t>();

    // Parse raw_data array
    JsonArray rawDataArray = doc["raw_data"];
    int i = 0;
    for (uint64_t value : rawDataArray) {
        if (i < RAW_DATA_ARRAY_SIZE) {
            data.DecodedRawDataArray[i++] = value;
        } else {
            break;  // Prevent overflow
        }
    }

    // Parse timing_info object
    JsonObject timingInfoJson = doc["timing_info"];
    data.timingInfo.HeaderMarkMicros = timingInfoJson["HeaderMarkMicros"].as<uint16_t>();
    data.timingInfo.HeaderSpaceMicros = timingInfoJson["HeaderSpaceMicros"].as<uint16_t>();
    data.timingInfo.OneMarkMicros = timingInfoJson["OneMarkMicros"].as<uint16_t>();
    data.timingInfo.OneSpaceMicros = timingInfoJson["OneSpaceMicros"].as<uint16_t>();
    data.timingInfo.ZeroMarkMicros = timingInfoJson["ZeroMarkMicros"].as<uint16_t>();
    data.timingInfo.ZeroSpaceMicros = timingInfoJson["ZeroSpaceMicros"].as<uint16_t>();

    Serial.println("Data successfully read from JSON file:");
    serializeJsonPretty(doc, Serial);

    file.close();
    return true;
}

#endif 
