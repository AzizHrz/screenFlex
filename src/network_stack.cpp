#include "network_stack.h"
#include "Mylog.h"
#include <ArduinoJson.h>
#include "schedule.h"
#include <esp_wifi.h>



NetworkStack::NetworkStack() : MyMacAddr{0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}
{
    WoLRepetion=10 ;
}
void NetworkStack::configureWiFi(const char *ssid = "", const char *passwd = "")
{
    #if ( OTA_WIFI_ETH == 1   )
    this->ssid = ssid;
    this->passwd = passwd;
    #endif
    }

void NetworkStack::configureOTA(bool enable)
{
    if (enable)
    {
        MyLog(INFO, "OTA Enabled ");
        isOTAEnabled = true;
    }
    else
    {
        MyLog(INFO, "OTA Disabled");
        MyLog(INFO, "Rollback Disabled");
    }
}
const char* NetworkStack::MacArrayToString(const uint8_t* arr, size_t _sizeMacString) {
    static char macString[18]; // 6*2 hex + 5 colons + null terminator = 18
    snprintf(macString, _sizeMacString, "%02X:%02X:%02X:%02X:%02X:%02X",
             arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
    return macString;
}

void NetworkStack::InitWifiModule(){ 
#if (ENABLE_WIFI == 1)
    /*
    We are Enabling the WiFi module for server communication,
    This macro is used also to speed up testing process when we don't need wifi
    */
    MyLog(INFO, "Enabling wifi connecting to a WiFi Station");

    WiFi.mode(WIFI_STA);
    MyLogF(INFO, "Connecting to %s \n\r", ssid);
    WiFi.begin(ssid, passwd);
    while (WiFi.status() != WL_CONNECTED) // TODO
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    MyLogF(INFO, "Connected to Network %s \n\r", ssid);

    LocalIP = WiFi.localIP();
    MyLog(INFO, "Ethernet Module is Disabled");
    MyLogF(INFO, "Local IP Address %s", LocalIP.toString());

#endif
}
/* // old init ethernet module version
void NetworkStack::InitEthernetModule(uint8_t cs_pin){ 
    #if (ENABLE_ETHERNET == 0)
    MyLog(INFO, "Ethernet Module is Disabled");
        

       // This macro is only for testing purposes when we don't need to test the WoL
        //to speed up the testing process
    
    return;
#else
    MyLog(INFO, "Enabling the Ethernet Module");

    Ethernet.init(cs_pin);
    Ethernet.begin(MyMacAddr);
    Serial.println("Testing eth  connection  ");
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        Serial.flush();
    }

    EthernetLinkStatus st;
    Serial.println("TEsting Link status ");
    st = Ethernet.linkStatus();
    while (st != LinkON)
    {
        delay(20);
        Serial.print(".");
        Serial.flush();
        st = Ethernet.linkStatus();
        switch (st)
        {
        case LinkON:
            Serial.println("LINK ON ");
            break;
        case Unknown:
            Serial.println("UNKNOW");
            break;
        case LinkOFF:
            Serial.println("LINK OFF");
            break;
        default:
            Serial.println("ERROR ... ");
            break;
        }
    }

    Serial.println("ETH connection success  ");
    delay(1000);
    Serial.println("Ethernet initialized");

    // LocalIP = WiFi.localIP();
#endif

//If the WiFi module is not enabled, we will be using the Ethernet Module
//for both WoL and server communication.

#if (ENABLE_WIFI == 0)
    Serial.print("IP Address: ");
    LocalIP = Ethernet.localIP();
    MyLogF(INFO, "Local IP Address %s", LocalIP.toString());
#endif
}

*/
// New version of InitEthernetModule
bool NetworkStack::InitEthernetModule(uint8_t cs_pin){ 
#if (ENABLE_ETHERNET == 0)
    MyLog(INFO, "Ethernet Module is Disabled");
    /*    

        This macro is only for testing purposes when we don't need to test the WoL
        to speed up the testing process
    */
    return false;
#else
     
    MyLog(INFO, "Enabling the Ethernet Module");

    Ethernet.init(cs_pin);
    Ethernet.begin(MyMacAddr);
    Serial.println("Testing eth  connection  ");
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        Serial.flush();
    }

    EthernetLinkStatus st;
    Serial.println("TEsting Link status ");
    st = Ethernet.linkStatus();
    static uint8_t attemptCount = 0;
    static unsigned long lastAttemptTime = 0;
    while (st != LinkON && attemptCount < 5 && millis() - lastAttemptTime > 20)
    {
        // Retry every 20 milliseconds
            lastAttemptTime = millis();
            attemptCount++;
            Serial.print(".");
            Serial.flush();
        
        
        st = Ethernet.linkStatus();
        switch (st)
        {
        case LinkON:
            Serial.println("LINK ON ");
            break;
        case Unknown:
            Serial.println("UNKNOW");
            break;
        case LinkOFF:
            Serial.println("LINK OFF");
            break;
        default:
            Serial.println("ERROR ... ");
            break;
        }
    }
    if (st != LinkON)
    {
        MyLog(ERROR, "Failed to establish Ethernet link after multiple attempts.");
        return false; // Return false if link could not be established
    }else
    {
        MyLog(INFO, "Ethernet link established successfully.");
        delay(1000);
        Serial.println("Ethernet initialized");
        return true; // Return true if link was established successfully
    }

    

    // LocalIP = WiFi.localIP();
#endif
/*
If the WiFi module is not enabled, we will be using the Ethernet Module
for both WoL and server communication.
*/
#if (ENABLE_WIFI == 0)
    Serial.print("IP Address: ");
    LocalIP = Ethernet.localIP();
    MyLogF(INFO, "Local IP Address %s", LocalIP.toString());
#endif
}


uint8_t *NetworkStack::GetMacaddr()
{

    return MyMacAddr;
}
//by aziz 
uint8_t* NetworkStack::GetESP32Macaddr()
{
    static uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    return mac;
}


String NetworkStack::HTTPGET(const String &reqPath)
{

    String response = "";
    if (client.connect(serverIP, ServerPort))
    {
        const String t = serverIP.toString();
        Serial.println("Connected to server");

        // Send the GET request
        client.println("GET " + reqPath + " HTTP/1.1");
        client.println("Host: " + t);
        client.println("Connection: close");
        client.println();

        // Read the server response
        response = ReadHTTPResponse();
        client.stop();
        Serial.println("Connection closed");
    }
    else
    {
        Serial.println("Connection failed");
    }
    return response;
}
String NetworkStack::HTTPPOST(const String &reqPath, const String &contentType, const String &data)
{
    String response = "";
    if (client.connect(serverIP, ServerPort))
    {
        const String t = serverIP.toString();
        Serial.println("Connected to server");

        // Send the POST request
        client.println("POST " + reqPath + " HTTP/1.1");
        client.println("Host: " + t);
        client.println("Connection: close");
        client.println("Content-Type: " + contentType);
        client.printf("Content-Length: %d\r\n", data.length());
        client.println(); // End of headers

        // Send the body
        client.println(data);

        // Read the server response
        response = ReadHTTPResponse();
        client.stop();
        Serial.println("Connection closed");
    }
    else
    {
        Serial.println("Connection failed");
    }
    return response;
}
String NetworkStack::ReadHTTPResponse()
{
    String response = "";
    bool isHeader = true;

    while (client.connected() || client.available())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            isHeader = false; // End of headers
        }
        else if (!isHeader)
        {
            response += line + "\n"; // Append body content
        }
    }
    return response;
}

void NetworkStack::enableOTA(bool state)
{
    isOTAEnabled = state;
}

void NetworkStack::WakeOnLan(const uint8_t TargetMac[], int repetition = 6)
{

    MyLogF(DEBUG, "Performing WakeOnLAn %d ...", repetition);
    MyLog(DEBUG, "WOL TARGET ");
    // for (int )
    IPAddress broadcastIP(255, 255, 255, 255);
    EthernetUDP udp;
    for (int i = 0; i < repetition; i++)
    {
        byte magicPacket[102];        // Magic packet: 6 FFs + 16 MAC addresses
        memset(magicPacket, 0xFF, 6); // Fill first 6 bytes with 0xFF

        // Append MAC address 16 times
        for (int i = 6; i < 102; i++)
        {
            magicPacket[i] = TargetMac[(i - 6) % 6];
        }

        // Send the packet
        udp.beginPacket(broadcastIP, 9); // Port 9 is standard for WoL
        udp.write(magicPacket, sizeof(magicPacket));
        udp.endPacket();
    }
}

const String NetworkStack::GetFirmwareVersion()
{
   // MyLogF(INFO, "Requsting Firmware version form Server %s", serverIP.toString());
    MyLogF(INFO, "Requsting Firmware version form Server %s", GetServerIP().toString());

#if (MyServ == 1)
    String response = HTTPGET("/version");
    Serial.println(response);
#else
    String response = HTTPGET("/api/v1/screenflex/box/version");
#endif
    // TODO: Check if response is empty
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        MyLogF(ERROR, "deserializeJson() failed: %s", error.f_str());
        return "";
    }

    bool success = doc["success"]; // true
    if (success == true)
    {
        const String softv = doc["data"]["version"].as<String>(); // "1.0.0"
        return softv;
    }
    else
        return "";
}

const String NetworkStack::getUserFromServer()
{
   // MyLogF(INFO, "Requsting Firmware version form Server %s", serverIP.toString());
    MyLogF(INFO, "Requsting Status & User form Server %s", GetServerIP().toString());

#if (MyServ == 1)
    String response = HTTPGET("/api/devices/status/02347789");
    Serial.println(response);
#else
    String response = HTTPGET("/api/v1/screenflex/box/version");
#endif
    // TODO: Check if response is empty
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        MyLogF(ERROR, "deserializeJson() GetUserFromServer() failed: %s", error.f_str());
        return "";
    }

    bool success = doc["success"]; // true
    String status = doc["status"].as<String>();
    if (success == true && status == "active")
    {
        const String user = doc["user"].as<String>(); // "1.0.0"
        return user;
    }
    else
        return "";
}

const String NetworkStack::pingTokenServer() {
    // Build JSON payload
    DynamicJsonDocument doc(1024);
    //doc["machineid"] = "MAC";
    doc["machineid"] = String(MacArrayToString(GetESP32Macaddr(), 18));
    doc["os"] = "esp32";

    String payload;
    serializeJson(doc, payload);

    // Make HTTP POST call using custom method
    String response = HTTPPOST("/api/devices/register", "application/json", payload);

    if (response.length() == 0) {
        MyLog(ERROR, "Empty response or connection failed.");
        return "false";
    }

    // Debug the raw response (optional)
    Serial.println("Raw Response:");
    Serial.println(response);
    

    
    DynamicJsonDocument resDoc(1024);
    DeserializationError error = deserializeJson(resDoc, response);

    if (error) {
        MyLogF(ERROR, "JSON parsing failed: %s", error.c_str());
        return "false";
    }

    if (resDoc["success"]) {
        String status = resDoc["status"].as<String>();
        String token = resDoc["token"].as<String>();

        MyLogF(INFO, "token: %s", token.c_str());
        MyLogF(INFO, "Status: %s", status.c_str());

        if (status == "active") {
            MyLog(INFO, "Access granted. Status is ACTIVE.");
        } else if (status == "pending") {
            MyLog(ERROR, "Access is still pending.");
        } else if (status == "revoked") {
            MyLog(ERROR, "Access has been revoked.");
        } else {
            MyLog(ERROR, "Unknown status received.");
        }

        return token; // Return the token
    } else {
        MyLog(ERROR, "Server responded with success=false.");
        return "false"; // Indicate failure
    }
}
const String NetworkStack::GetSchedule_HTTP(){ 

    MyLogF(INFO, "Requsting Firmware version form Server %s", GetServerIP().toString());
#if (MyServ == 1)
    String response = HTTPGET("/1/schedule");
    Serial.println(response);
#else
    String response = HTTPGET("/api/v1/screenflex/box/version");
#endif
    // TODO: Check if response is empty
    // StaticJsonDocument<96> doc;

    // DeserializationError error = deserializeJson(doc, response);

    // if (error)
    // {
    //     MyLogF(ERROR, "deserializeJson() failed: %s", error.f_str());
    //     return "";
    // }
    return response ; 
    // bool success = doc["success"]; // true
    // if (success == true)
    // {
    //     const String softv = doc["data"]["version"].as<String>(); // "1.0.0"
    //     return softv;
    // }
    // else
    //     return "";
} 
bool NetworkStack::OTA()
{
    if (!isOTAEnabled)
    {
        return false;
    }
    const String newSoftV = GetFirmwareVersion();
    if ( !( strcmp(newSoftV.c_str()  , TEST_SOFTWARE_VERSION) == 0 ) )
    { // TODO enhance comaprison
        MyLogF(INFO, "Detected a new version from sevrer %s with version %s", GetServerIP().toString(), newSoftV);
        MyLog(INFO, "Perfroming OTA, Dowloading Frimware now ...");
#if (MyServ == 1)
        const String reqPath = "/update";
#else
        const String reqPath = "/screenflex/firmware.bin";
#endif
        int FirmwareSize = RequestOTA(reqPath);
        if (FirmwareSize < 0)
        {
            // MyLogF(ERROR,"ERROR %d , Error Requesting OTA ...",FirmwareSize );
            return false;
        }else{
            int state = UpdateFirmware(FirmwareSize);
            if (state < 0){
                return false;
            }else{
                MyLog(INFO, "Ready to update, just restart ...");
                if (Update.canRollBack()){
                    MyLog(INFO, "Rollback enabled ");
                }else{
                    MyLog(ERROR, "enabled To enable rollback ");
                }
                return true;
            }
        }
    }
    else
    {
        MyLogF(INFO, "No new version from sevrer %s with version %s", GetServerIP().toString(), newSoftV);
        MyLog(INFO, "Ignoring OTA.");
        return false;
    }
    }
    int NetworkStack::RequestOTA(const String &reqPath)
    {
        // TODO: ADD MD5 Check

        // Connect to the server
        if (client.connect(serverIP, ServerPort))
        {
            Serial.println("Connected to OTA server");

            // Send HTTP GET request
            client.println("GET " + reqPath + " HTTP/1.1");
            client.println("Host: 203.161.60.31");
            client.println("Connection: close");
            client.println();

            // Wait for the server response
            int contentLength = -1;
            while (client.connected())
            {
                String line = client.readStringUntil('\n');
                Serial.println(line);
                if (line.startsWith("Content-Length:"))
                {
                    contentLength = line.substring(15).toInt();
                }
                if (line == "\r")
                {
                    MyLog(DEBUG, "Finished  receiving Headers");
                    break; // End of headers
                }
            }

            if (contentLength <= 0)
            {
                MyLog(ERROR, "Invalid content length. OTA update aborted.");
                client.stop();
                return -1;
            }
            else
            {
                return contentLength;
            }
        }
        else
        {
            MyLogF(ERROR, "Unable to connect to Server: %s, port %d", serverIP.toString(), ServerPort);
            client.stop();
            return -2;
        }
    }
int NetworkStack::UpdateFirmware(int FirmwareSize)
{
    MyLogF(DEBUG, "New Firmware size %d", FirmwareSize);

    // Initialize OTA update
    if (Update.begin(FirmwareSize))
    {
        size_t written = 0;
        uint8_t buff[128];
        unsigned long timeout = millis();
        int i = 0;
        while ((client.connected() || client.available()) && written < FirmwareSize)
        {
            if (client.available())
            {
                size_t len = client.readBytes(buff, sizeof(buff));
                if (len > 0)
                {
                    written += Update.write(buff, len);
                    if ((written * 100) / FirmwareSize >= i)
                    {
                        Serial.printf("%d%% : Written %d/%d bytes...\n\r", (written * 100) / FirmwareSize, written, FirmwareSize);
                        i += 10;
                    }
                }
            }
        }
        if (written == FirmwareSize)
        {
            if (Update.end())
            {
                MyLog(INFO, "OTA Update completed successfully.");
                return 1;
            }
            else
            {
                MyLogF(ERROR, "OTA Update failed: %s\n", Update.errorString());
                return -1;
            }
        }
        else
        {
            MyLogF(ERROR, "OTA Update failed: Incomplete data received. %s", Update.errorString());
            Update.abort();
            return -1;
        }
    }
    else
    {
        MyLog(ERROR, "Not enough space for OTA update.");
        MyLog(DEBUG, "closing connection.");
        client.stop();
        return -2;
    }
}
