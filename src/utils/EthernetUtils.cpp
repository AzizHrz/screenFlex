#include "utils/EthernetUtils.h"
#include "utils/Mylog.h"
#include <Ethernet.h>
#include <SPI.h>

extern byte mac[];
extern int W5500_CS_PIN;

bool initEthernet() {
    static unsigned long lastAttempt = 0;
    const unsigned long RETRY_INTERVAL = 60000;

    if (millis() - lastAttempt < RETRY_INTERVAL && lastAttempt != 0) {
        return false;
    }

    lastAttempt = millis();

    pinMode(W5500_CS_PIN, OUTPUT);
    digitalWrite(W5500_CS_PIN, HIGH);
    SPI.begin();

    Ethernet.init(W5500_CS_PIN);
    MyLog(INFO, "Starting Ethernet connection...");

    if (Ethernet.begin(mac, 10000) == 0) {
        MyLog(ERROR, "Failed to configure Ethernet using DHCP");
        return false;
    }

    MyLogF(INFO, "Ethernet connected! IP: %s", Ethernet.localIP().toString().c_str());
    return true;
}
