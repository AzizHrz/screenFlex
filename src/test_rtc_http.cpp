#include "conf.h"
#include "soft_version.h"
#include "Hardware.h"
#include "RGBModule.h"
#include "ButtonModule.h"
#include "IR_Module.h"
#include "Mylog.h"
#include <Arduino.h>
#include <RtcDateTime.h>
#include <RTC_module.h>
#include <LittleFS.h>
#include <time.h>
#include <WString.h>
#include <IRremote.h>


#define IR_SEND_PIN 4

// RTC and Schedule Configuration
RTC_Module rtc;
RtcDateTime beginTime(2022, 1, 1, 8, 0, 0); // Start time: 8:00 AM
RtcDateTime endTime(2028, 1, 1, 18, 0, 0); // End time: 6:00 PM
String beginDay = "Monday ";              // Start day of the week
String endDay = "Saturday";                  // End day of the week
String tvState = "OFF";                    // Default TV state

// Days of the week as strings
const char* daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void switchTV_State(uint8_t* TV) {
    *TV = (*TV == 0) ? 1 : 0;
}

// Hardware Modules
IRHandler irHandler(IR_RECEIVE_PIN, IR_SEND_PIN_, TV_STATUS_PIN);
ButtonModule btn(PIN_BTN);
RGBModule rgb(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);

// Global Variables
volatile uint8_t Blinking_status = BLUE_COLOR_STATUS; // RGB status
unsigned long btime = 0;                             // Button press time
uint8_t TV = 0;                                      // TV state (1 = ON, 0 = OFF)

// Function Prototypes
void manageTVState(uint8_t TV, String &tvState, bool isOperating);
int findIndex(const char* arr[], int size, String target);
String get_day_of_week();

/*
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
*/
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

    // Initialize hardware modules
    irHandler.begin();
    // irHandler.init_littleFS(); // Initialize LittleFS for IR data storage
    rgb.RGBInit();             // Initialize RGB LED
    rgb.setBlinkingStatus(BLUE_COLOR_STATUS);
    // btn.InitButton();          // Initialize button
    rgb.runRGBTask();          // Start RGB task

    // Initialize RTC module
    rtc.RTC_Module_init();
    rtc.set_beginTime(beginTime);
    rtc.set_endTime(endTime);

    IrSender.begin(IR_SEND_PIN); 

    MyLog(INFO, "Finished Initializing all the Modules...");
    MyLog(INFO, "Starting Now...");

     // Compare current day with scheduled days
    int d = findIndex(daysOfWeek, 7, get_day_of_week());
    int b = findIndex(daysOfWeek, 7, beginDay);
    int e = findIndex(daysOfWeek, 7, endDay);

    // Handle wrap-around for days (e.g., Friday to Monday)
    if (e <= b) {
        e += 7;
        d += 7;
    }

    // Check if current day is within the scheduled range
    delay(2000);
    bool isWithinSchedule = (d >= b && d <= e);

    RtcDateTime currentTime = rtc.RTC_getDateTime();






    bool isOperating = rtc.isWithinOperatingPeriod(currentTime, isWithinSchedule);
    if (isOperating) {
        printf("Inside operating time\n");
        if (TV == 0) {
            MyLog(DEBUG, "TV is OFF during operation time, turning it ON.");
                sendIR(); // Send IR signal to turn TV ON
                switchTV_State(&TV); // Change TV state **only after sending IR**
        } else {
            MyLog(DEBUG, "TV is already ON. No action needed.");
        }
    } else {
        printf("Outside operating time\n");
        if (TV == 1) {
            MyLog(DEBUG, "TV is ON outside operation time, turning it OFF.");
                sendIR(); // Send IR signal to turn TV OFF
                switchTV_State(&TV); // Change TV state **only after sending IR**
        } else {
            MyLog(DEBUG, "TV is already OFF. No action needed.");
        }
    }





/*
    // fasa5 hetha
                sendIR(); // Send IR signal to turn TV ON
*/     



 /*   
    // Wait for 5 seconds to receive an IR signal
    unsigned long startTime = millis();
    bool receivedSignal = false;

    while (millis() - startTime < 5000) {
        if (irHandler.receiveIR(false)) { // Receive IR signal without saving
            MyLog(INFO, "IR Signal received during startup. Saving to LittleFS...");
            auto receivedData = irHandler.getReceivedData();
            irHandler.storeData(receivedData); // Save received data
            irHandler.PrintMyIRData(receivedData); // Debug print
            receivedSignal = true;
            break; // Exit the loop once data is received and stored
        }
        delay(100); // Avoid excessive loop iterations
    }

    if (receivedSignal) {
        MyLog(INFO, "IR data received and stored in LittleFS.");
    } else {
        MyLog(INFO, "No IR signal received during startup. Checking LittleFS for stored data...");
        if (irHandler.IsIRDataavailable(f)) {
            MyLog(INFO, "IR data found in LittleFS. Using stored data.");
        } else {
            MyLog(ERROR, "No IR data available in LittleFS. Please send a signal via button press.");
        }
    }

    // Set initial button press time
    btime = millis();
    
*/
}

void loop() {
    /*
    // Handle button press (5-second long press to receive new IR signal)
    btn.HandleButton_5s();
    if (btn.isLongPressed_5s()) {
        MyLog(INFO, "Button PRESSED for more than 5s...");
        rgb.setBlinkingStatus(RED_COLOR_STATUS);
        irHandler.clear_file(f); // Clear stored IR data
        if (irHandler.receiveIR(true)) { // Receive new IR signal and save it
            MyLog(INFO, "New IR signal received and stored in LittleFS.");
            rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
        } else {
            MyLog(ERROR, "Failed to receive new IR signal.");
            rgb.setBlinkingStatus(RED_COLOR_STATUS);
        }
        btime = millis(); // Reset button press time
    }

    // RGB blinking logic for 3 seconds after button press
    if (millis() - btime <= 3000) {
        rgb.setBlinkingStatus(GREEN_COLOR_STATUS);
    } else {
        rgb.setBlinkingStatus(NO_COLOR_STATUS);
    }
*/







    // Get current RTC time
    RtcDateTime currentTime = rtc.RTC_getDateTime();
    if (currentTime.Year() < 2023) {
        MyLog(ERROR, "RTC not initialized. Skipping operating period check.");
        return;
    }

    // Log current RTC time
    MyLog(INFO, ("Current RTC Time: " + rtc.rtcDateTimeToString(currentTime)).c_str());

    // Compare current day with scheduled days
    int d = findIndex(daysOfWeek, 7, get_day_of_week());
    int b = findIndex(daysOfWeek, 7, beginDay);
    int e = findIndex(daysOfWeek, 7, endDay);

    // Handle wrap-around for days (e.g., Friday to Monday)
    if (e <= b) {
        e += 7;
        d += 7;
    }

    // Check if current day is within the scheduled range
    bool isWithinSchedule = (d >= b && d <= e);
    MyLog(INFO, isWithinSchedule ? "Inside scheduled days." : "Outside scheduled days.");

    // Check if current time is within the operating period
    bool isOperating = rtc.isWithinOperatingPeriod(currentTime, isWithinSchedule);


    // Manage TV state based on schedule and current time
    if (isOperating) {
        printf("Inside operating time\n");
        if (TV == 0) {
            MyLog(DEBUG, "TV is OFF during operation time, turning it ON.");
                sendIR(); // Send IR signal to turn TV ON
                switchTV_State(&TV); // Change TV state **only after sending IR**
        } else {
            MyLog(DEBUG, "TV is already ON. No action needed.");
        }
    } else {
        printf("Outside operating time\n");
        if (TV == 1) {
            MyLog(DEBUG, "TV is ON outside operation time, turning it OFF.");
                sendIR(); // Send IR signal to turn TV OFF
                switchTV_State(&TV); // Change TV state **only after sending IR**
        } else {
            MyLog(DEBUG, "TV is already OFF. No action needed.");
        }
    }

    printf("TV state: %d\n", TV);

    delay(5000);


/*
    delay(600000);
    Serial.println("resend");
    sendIR(); // Send IR signal to turn TV ON
    // Delay to avoid excessive loop iterations
    
*/


}

// Manage TV state based on schedule and current time
void manageTVState(uint8_t TV, String &tvState, bool isOperating) {
    if (isOperating) {
        printf("Inside operating time\n");
        if (TV == 0) {
            MyLog(DEBUG, "TV is OFF during operation time, turning it ON.");
                sendIR(); // Send IR signal to turn TV ON
                switchTV_State(&TV); // Change TV state **only after sending IR**
        } else {
            MyLog(DEBUG, "TV is already ON. No action needed.");
        }
    } else {
        printf("Outside operating time\n");
        if (TV == 1) {
            MyLog(DEBUG, "TV is ON outside operation time, turning it OFF.");
                sendIR(); // Send IR signal to turn TV OFF
                switchTV_State(&TV); // Change TV state **only after sending IR**
        } else {
            MyLog(DEBUG, "TV is already OFF. No action needed.");
        }
    }
}

// Find the index of a day in the daysOfWeek array
int findIndex(const char* arr[], int size, String target) {
    for (int i = 0; i < size; i++) {
        if (strcmp(arr[i], target.c_str()) == 0) {
            return i;
        }
    }
    return -1; // Return -1 if not found
}

// Get the current day of the week as a string
String get_day_of_week() {
    RtcDateTime now = rtc.RTC_getDateTime();
    int dayOfWeek = now.DayOfWeek(); // Assuming RTC provides day of the week
    return daysOfWeek[dayOfWeek];
}