#include <SPI.h>
#include <Ethernet_Generic.h>  // Ensure the library is installed via Arduino Library Manager

#define PIN_MOSI 23
#define PIN_MISO 19
#define PIN_SCK  18
#define PIN_CS   33

SPIClass SPI_eth(VSPI);
#define eth Ethernet

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🚀 Booting up...");

  SPI_eth.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  Serial.println("🧪 SPI started...");

  Serial.println("📡 Trying Ethernet...");
  if (!eth.begin(mac)) {
    Serial.println("❌ Ethernet failed to initialize!");
  } else {
    Serial.println("✅ Ethernet initialized!");
    Serial.print("IP: ");
    Serial.println(eth.localIP());
  }
}

void loop() {
  // Try pinging it if you get an IP
}
