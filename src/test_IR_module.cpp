#include <IRremote.h> // Include the IRremote library
#include "Hardware.h"

// Define the IR_SEND_PIN macro with the appropriate pin number

#define IR_SEND_PIN 4 // Pin for sending IR signals 
 
// Define the MyIRData struct
typedef struct {
    bool data_stored;
    decode_type_t protocol;
    IRRawDataType DecodedRawDataArray[RAW_DATA_ARRAY_SIZE];
    DistanceWidthTimingInfoStruct timingInfo;
    uint8_t numberOfBits;
    uint16_t IR_Adress;
    uint16_t IR_Command;
} MyIRData;

// Initialize the MyIRData struct globally with your data
MyIRData myIRData = {
    .data_stored = true,
    .protocol = PULSE_DISTANCE, // Replace with the correct protocol type
    .DecodedRawDataArray = {11208015, 0, 0, 0, 0, 0},
    .timingInfo = {
        .HeaderMarkMicros = 4000,
        .HeaderSpaceMicros = 4000,
        .OneMarkMicros = 500,
        .OneSpaceMicros = 2000,
        .ZeroMarkMicros = 500,
        .ZeroSpaceMicros = 1000
    },
    .numberOfBits = 24,
    .IR_Adress = 0,
    .IR_Command = 0
};

// Function to send IR signal
void sendIR() {
    IrReceiver.stop(); // Stop the IR receiver to avoid interference
    Serial.println("Sending data...");

    // Send the IR signal based on the protocol
    if (myIRData.protocol == PULSE_DISTANCE) {
        Serial.println("SENDING PULSE DISTANCE PROTOCOL");
        IrSender.sendPulseDistanceWidthFromArray(38, &myIRData.timingInfo, &myIRData.DecodedRawDataArray[0], myIRData.numberOfBits, PROTOCOL_IS_LSB_FIRST, 100, 0);
    } else if (myIRData.protocol == NEC) {
        Serial.println("SENDING NEC PROTOCOL");
        IrSender.sendNEC(myIRData.IR_Adress, myIRData.IR_Command, 38);
    } else if (myIRData.protocol == SAMSUNG) {
        Serial.println("SENDING SAMSUNG PROTOCOL");
        IrSender.sendSamsung(myIRData.IR_Adress, myIRData.IR_Command, 38);
    } else if (myIRData.protocol == LG) {
        Serial.println("SENDING LG PROTOCOL");
        IrSender.sendLG(myIRData.IR_Adress, myIRData.IR_Command, 38);
    } else {
        Serial.println("Unknown protocol, cannot send. Protocol not supported.");
    }

    delay(100); // Add a small delay to ensure the signal is sent
    IrReceiver.start(); // Restart the IR receiver
}

void setup() {
    Serial.begin(115200);
    IrReceiver.begin(IR_RECEIVE_PIN); // Initialize the IR receiver (if needed)
    IrSender.begin(IR_SEND_PIN); // Initialize the IR sender

    sendIR(); // Send the IR signal
}

void loop() {
    // Your main code here
}