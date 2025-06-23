#include <Wire.h> // For I2C communication with RTC
#include <Arduino.h> 
#include <IRremote.h> 
#define IR_SEND_PIN 4         // Pin connected to IR Transmitter
#define IR_RECEIVE_PIN 16     // Pin connected to IR Receiver
#define RTC_DAT 21            // RTC Data pin
#define RTC_CLK 22            // RTC Clock pin
#define RTC_RST 14            // RTC Reset pin

// RTC Address (DS1307 typically uses 0x68)
#define RTC_ADDRESS 0x68

// IR Receiver setup
IRrecv irrecv(IR_RECEIVE_PIN); // Create an IR receiver object
decode_results results;        // Structure to store received IR data

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging

  // Initialize IR receiver
  irrecv.enableIRIn();

  // Initialize RTC
  Wire.begin(); // Initialize I2C communication
  if (!isRTCRunning()) {
    Serial.println("RTC is NOT running, setting the time!");
    setRTCDateTime(2023, 10, 5, 12, 0, 0); // Set initial date and time (YYYY, MM, DD, HH, MM, SS)
  }

  Serial.println("Setup complete.");
}

void loop() {
  // Display the current time from RTC
  displayCurrentTime();

  // Check if IR receiver has detected a signal
  if (irrecv.decode(&results)) {
    Serial.print("IR signal detected: ");
    Serial.print(results.value, HEX); // Print the received IR signal in hexadecimal format
    Serial.println();
    displayCurrentTime(); // Send the current time when IR signal is detected
    irrecv.resume();      // Resume receiving IR signals
  }

  delay(1000); // Wait for 1 second before checking again
}

// Function to check if RTC is running
bool isRTCRunning() {
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00); // Select seconds register
  Wire.endTransmission();
  Wire.requestFrom(RTC_ADDRESS, 1);
  uint8_t second = Wire.read();
  return !(second == 0x00); // If seconds are 0x00, RTC might not be running
}

// Function to set the RTC date and time
void setRTCDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00); // Start at address 0 (seconds)
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(0)); // Day of the week (not used, set to 0)
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year - 2000)); // Year since 2000
  Wire.endTransmission();
}

// Function to convert decimal to BCD (Binary-Coded Decimal)
uint8_t decToBcd(uint8_t val) {
  return ((val / 10 * 16) + (val % 10));
}

// Function to convert BCD to decimal
uint8_t bcdToDec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
}

// Function to display the current time from RTC
void displayCurrentTime() {
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00); // Start at address 0 (seconds)
  Wire.endTransmission();
  Wire.requestFrom(RTC_ADDRESS, 7);

  uint8_t second = bcdToDec(Wire.read());
  uint8_t minute = bcdToDec(Wire.read());
  uint8_t hour = bcdToDec(Wire.read() & 0b111111); // Ignore high-order bits for 12/24-hour mode
  uint8_t dayOfWeek = bcdToDec(Wire.read());
  uint8_t day = bcdToDec(Wire.read());
  uint8_t month = bcdToDec(Wire.read());
  uint16_t year = bcdToDec(Wire.read()) + 2000;

  Serial.print("Current Time: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.print(second);
  Serial.print(" | Date: ");
  Serial.print(day);
  Serial.print("/");
  Serial.print(month);
  Serial.print("/");
  Serial.println(year);
}