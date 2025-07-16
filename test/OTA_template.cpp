#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h>
#include <Ethernet.h>
#include <HTTPClient.h>
#define WIFI 1
#define PORT 3500
#ifdef ETH
#include "wakeOnLan.h"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient client;
#elif defined(WIFI)
#include <WiFi.h>
const char *ssid = "TT_23C0";        //"TP-Link_AP_70CA";
const char *password = "i6q4k3b21r"; //"97800903";
WiFiClient client;
#endif
// MAC address for your Ethernet shield (should be unique)
// 192.168.1.14:5000// 192.168.1.14 // 203.161.60.31:3500
IPAddress server(203, 161, 60, 31); // Example: IP for example.com (or use a domain name)

// function defines
void performGET();
void performOTAUpdate();
void file_update(const char *filePath);
void performOTAUpdateWIFI();

void setup()
{
    Serial.begin(115200);

#ifdef ETH
    Ethernet.init(33);
    Ethernet.begin(mac);
    Serial.println("TEsting eth  connection  ");
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
            Serial.println("Default ");
            break;
        }
    }
    Serial.println("ETH connection success  ");
    delay(1000);
    Serial.println("Ethernet initialized");
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());

#elif defined(WIFI)

    WiFi.mode(WIFI_STA);
    Serial.printf("COnnecting to %s \n\r", ssid);
    WiFi.begin(ssid, password); // make sure to enter your wifi credentials on "credential.h"
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.printf("COnnected to %s \n\r", ssid);

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
#endif
    // Start the Ethernet connection
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println("LittleFS initialized");
    // Give the Ethernet shield time to initialize
    bool state = Update.canRollBack();
    Serial.println(state == true ? "We can rollback " : "cannot rollback ");
    // Make a GET request
    // file_update("/settings.txt");
    const String t = server.toString();
    Serial.printf("connected to server %s\n\r", t);
    // performGET();
    Serial.println("DONE ");
    delay(3000);
    Serial.println("DONE ");

    performOTAUpdate();
}
void performPost();

void loop()
{
    // Read the server response
    // performGET();
    // delay(3000);
    // performPost();
    // delay(3000);
    delay(2000);
    Serial.println("Y");
}
void file_update(const char *filePath)
{
    File file = LittleFS.open(filePath, "r");

    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    size_t fileSize = file.size();
    Serial.printf("File size: %d bytes\n", fileSize);
    String reqPath = "/upload";

    if (client.connect(server, PORT))
    {
        Serial.println("Connected to server");
        // Send POST request headers
        client.println("POST " + reqPath + " HTTP/1.1");
        // client.println("Host: " + String(server));
        client.println("Connection: close");
        client.println("Content-Type: application/octet-stream");
        client.printf("Content-Length: %d\r\n", fileSize);

        // End headers
        client.println();

        // Send the file data
        size_t bytesSent = 0;
        uint8_t buffer[128]; // Buffer for reading file chunks

        while (file.available())
        {
            size_t bytesRead = file.read(buffer, sizeof(buffer)); // Read a chunk of the file
            client.write(buffer, bytesRead);                      // Send the chunk to the server
            bytesSent += bytesRead;
            Serial.printf("Sent %d/%d bytes\n", bytesSent, fileSize);
        }

        file.close();
        Serial.println("File upload complete");

        // Wait for server response
        while (client.connected())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
            if (line == "\r")
            {
                Serial.println("Headers Received");
                break;
            }
        }

        Serial.println("Response Body:");
        while (client.available())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
        client.stop();
        Serial.println("Connection closed");
    }
    else
    {
        Serial.println("Connection failed");
        file.close();
    }
}
void performPost()
{
    String QuerryParams = "?key1=value1&key2=value2";
    String reqPath = "/receive";
    String postData = "{\"key\":\"value\", \"number\":123}";
    String test = "POST /receive HTTP/1.1";
    if (client.connect(server, PORT))
    {
        // send psot reques
        Serial.println("Connected to server");
        // client.println("GET /test HTTP/1.1");
        client.println("POST " + reqPath + " HTTP/1.1");
        client.println("Connection: close");
        client.println("Content-Type: application/json");
        client.printf("Content-Length: %d\r\n", postData.length()); // Length of the post data

        // END of headers
        client.println();

        // Send the POST body
        client.println(postData); // Send the actual data to the server

        // wait for sever response
        while (client.connected())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
            if (line == "\r")
            {
                Serial.println("Headers Received");
                break;
            }
        }
        Serial.println("Reasponse Body: ");
        while (client.available())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
        client.stop();
        Serial.println("Connection closed ");
    }
    else
    {
        Serial.println("Connection failed");

        return;
    }
}

void performGET()
{
    // http://203.161.60.31:3500/api/v1/screenflex/box/version

    // String QuerryParams = "?key1=value1&key2=value2";
    String reqPath = "/api/v1/screenflex/box/version"; // +QuerryParams;
    // String test="GET /receive HTTP/1.1";
    if (client.connect(server, PORT))
    {
        Serial.println("Connected to server");
        // client.println("GET /test HTTP/1.1");
        client.println("GET " + reqPath + " HTTP/1.1");
        // client.println(test);
        client.println("Host: 203.161.60.31");

        // client.printf("Host: %s\n\r",server); // Use the domain name of your server
        client.println("Connection: close");
        client.println();
        // wait for sever response
        while (client.connected())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
            if (line == "\r")
            {
                Serial.println("Headers Received");
                break;
            }
        }
        Serial.println("Reasponse Body: ");
        while (client.available())
        {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
        client.stop();
        Serial.println("Connection closed ");
    }
    else
    {
        Serial.println("Connection failed");
        return;
    }
}

void performOTAUpdate()
{
    // EthernetClient client;
    // String serverIP = "172.16.121.101";
    // http://203.161.60.31:3500/screenflex/firmware.bin

    int serverPort = PORT;
    String firmwarePath = "/screenflex/firmware.bin";

    // Connect to the server
    if (client.connect(server, serverPort))
    {
        Serial.println("Connected to OTA server");

        // Send HTTP GET request
        client.println("GET " + firmwarePath + " HTTP/1.1");
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
                Serial.println("Headers received");
                break; // End of headers
            }
        }

        if (contentLength <= 0)
        {
            Serial.println("Invalid content length. OTA update aborted.");
            return;
        }

        Serial.printf("Starting OTA update, content length: %d bytes\n\r", contentLength);

        // Initialize OTA update
if (Update.begin(contentLength)) {
    size_t written = 0;
    uint8_t buff[128];
    unsigned long timeout = millis();
    int i = 0 ; 
    while ((client.connected() || client.available()) && written < contentLength) {
        if (client.available()) {
            size_t len = client.readBytes(buff, sizeof(buff));
            if (len > 0) {
                written += Update.write(buff, len);
                if ( (written * 100) / contentLength >=  i ){ 
                Serial.printf("%d%% : Written %d/%d bytes...\n\r", (written * 100) / contentLength, written, contentLength);
                i+=10; 
                }
            }
        }   
    }
    if (written == contentLength) {
        if (Update.end()) {
            Serial.println("OTA Update completed successfully. Restarting...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.printf("OTA Update failed: %s\n", Update.errorString());
        }
    } else {
        Serial.println("OTA Update failed: Incomplete data received.");
        Update.abort();
    }
} else {
    Serial.printf("Update.begin failed: %s\n", Update.errorString());
}
    }
    else
    {
        Serial.println("Failed to connect to OTA server.");
    }

    client.stop();
    Serial.println("Connection closed.");
}

void performOTAUpdateWIFI()
{
    HTTPClient http;
    const char *firmwareUrl =  "http://203.161.60.31:3500/screenflex/firmware.bin"; // "http://192.168.1.14:5000/update";
        // String firmwarePath = "/screenflex/firmware.bin";

    http.begin(firmwareUrl);   // Connect to the server
    int httpCode = http.GET(); // Request the binary file

    if (httpCode == HTTP_CODE_OK)
    {
        int contentLength = http.getSize();
        WiFiClient *stream = http.getStreamPtr();

        if (contentLength > 0)
        {
            Serial.printf("Starting OTA update, content length: %d bytes\n", contentLength);

            if (Update.begin(contentLength))
            {
                size_t written = Update.writeStream(*stream);

                if (written == contentLength)
                {
                    Serial.println("OTA update successful, restarting...");
                    if (Update.end())
                    {
                        Serial.println("Update finished.");
                        bool x = Update.rollBack();
                        Serial.println(x == true ? "ROLLBACK enabled " : "Rollback not enabled");
                        delay(1000);
                        ESP.restart();
                    }
                    else
                    {
                        Serial.printf("Update failed: %s\n", Update.errorString());
                    }
                }
                else
                {
                    Serial.printf("Written only %d/%d bytes. OTA update failed.\n", written, contentLength);
                }
            }
            else
            {
                Serial.println("Not enough space for OTA update.");
            }
        }
        else
        {
            Serial.println("Invalid content length.");
        }
    }
    else
    {
        Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    }
    http.end();
}