#include <Arduino.h>
#include "network_stack.h"
#include "Mylog.h"
#include "conf.h"
#include "RGBModule.h"
#include "Hardware.h"
NetworkStack Network;
// myserv: (192,168,1,14) , 5000
// bilel: (203,161,60,31) , 3500
//const char *password = "i6q4k3b21r"; //"97800903";
//const char *ssid = "TT_23C0";        //"TP-Link_AP_70CA";
const char* ssid = "Aziz_s_iPhone";
const char* password = "just100$";


RGBModule rgb(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);

void setup()
{
    Serial.begin(115200);


   // init RGB
   rgb.RGBInit();
   rgb.setBlinkingStatus(YELLOW_COLOR_STATUS);

#ifdef MyServ == 1
    //IPAddress ip(192, 168, 1, 14);
    IPAddress ip(SERVER_IP);
    
    Network.setServerIp(ip);
    Network.setSercerPort(5000);
#else
    IPAddress ip(203, 161, 60, 31);
    Network.setServerIp(ip);
    Network.setSercerPort(3500);
#endif
    Network.configureOTA(true);
    Network.configureWiFi(ssid, password);
//    Network.InitModule();
    Network.InitWifiModule();

    // Network.
    delay(3000);
    MyLog(INFO, "DONE");
    Serial.println(Network.GetFirmwareVersion());
    MyLog(INFO, "Performing OTA ");
    bool st = Network.OTA();
    if (st)
    {
        MyLog(INFO, "RESTATRING OTA ");
        ESP.restart();
    }
    else
    {
        MyLog(INFO, " OTA Failed miserably ");
    }

    // Serial.println(100/0);
}

void loop()
{
   rgb.setBlinkingStatus(RED_COLOR_STATUS);

}