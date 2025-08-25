/**
 * @file network_stack.h
 * @brief Network stack for handling Ethernet, Wi-Fi, OTA updates, and server communication.
 *
 * This module provides network functionalities, including initialization of Ethernet and Wi-Fi,
 * Wake-on-LAN (WoL), OTA updates, and HTTP communication.
 *
 * @author Moktar SELLAMI
 * @date 2025-02-16
 */
#ifndef NETWORK_STACK_H
#define NETWORK_STACK_H

#include <Arduino.h>
#include <LittleFS.h>
#include <Update.h>
#include <Ethernet.h>
#include <HTTPClient.h>
#include "conf.h"
#include "Mylog.h"

/**
 * @class NetworkStack
 * @brief Manages network communication, OTA updates, and Wake-on-LAN.
 */
class NetworkStack
{
public:
    const String  pingTokenServer();
    const String getUserFromServer(const String &token);
    uint8_t* GetESP32Macaddr();


    /**
     * @brief Constructor for NetworkStack.
     */
    NetworkStack();

    /**
     * @brief Sets the server IP address.
     * @param ip The IP address of the server.
     */
    void setServerIp(IPAddress ip)
    {
        MyLogF(INFO, "Updating IP address to %s ", ip.toString());
        serverIP = ip;
    }

    /**
     * @brief Sets the server port.
     * @param port The port number of the server.
     */
    void setSercerPort(int port) { ServerPort = port; }

    /**
     * @brief Configures Wi-Fi with SSID and password.
     * @param ssid The SSID of the Wi-Fi network.
     * @param passwd The password for the Wi-Fi network.
     */
    void configureWiFi(const char *ssid, const char *passwd);

    /**
     * @brief Enables or disables OTA (Over-The-Air) updates.
     * @param Enable If true, OTA updates are enabled.
     */
    void configureOTA(bool Enable);

    /**
     * @brief Initializes the Wi-Fi module.
     */
    void InitWifiModule();

        /**
     * @brief Initializes the Wi-Fi module as both ESP32 Access Point and Station
     */
    void InitWifi_AP_STA_Module();

    /**
     * @brief Initializes the Ethernet module with a specific chip select (CS) pin.
     * @param cs_pin The chip select pin for the Ethernet module.
     */
    // void InitEthernetModule(uint8_t cs_pin);

    bool InitEthernetModule(uint8_t cs_pin);

    /**
     * @brief Enables or disables OTA updates.
     * @param state If true, OTA updates are enabled.
     */
    void enableOTA(bool state);

    /**
     * @brief Performs an OTA update.
     * @return True if the update was successful, false otherwise.
     */
    bool OTA();

    /**
     * @brief Notifies the server that we are connected.
     * @todo to be implemented
     */
    void NotifyServer();

    /**
     * @brief Retrieves the firmware version from the server.
     * @return The firmware version as a string.
     */
    const String GetFirmwareVersion();
    /**
     * @brief Retrieves schedule from server.
     * @return The schedule as a string.
     */
    const String GetSchedule_HTTP();
    /**
     * @brief Sends a Wake-on-LAN (WoL) signal to a target device.
     * @param TagrgetMac The MAC address of the target device.
     * @param repetition The number of times to send the WoL packet.
     */
    void WakeOnLan(const uint8_t TagrgetMac[], int repetition);

    /**
     * @brief Initializes the Wake-on-LAN task.
     */
    void initTaskWol();

    /**
     * @brief Sets the number of repetitions for Wake-on-LAN.
     * @param n Number of repetitions.
     */
    void setWoLRepetiton(uint8_t n) { WoLRepetion = n; }

    /**
     * @brief Gets the configured server IP address.
     * @return The server IP address.
     */
    IPAddress GetServerIP() { return serverIP; }

    /**
     * @brief Retrieves the MAC address of the device.
     * @return Pointer to the MAC address array.
     */
    uint8_t *GetMacaddr();

     /**
     * @brief Converts a MAC address array to a string format.
     * @param arr The MAC address array.
     * @param macString The output string buffer.
     * @param _sizeMacString The size of the output buffer.
     */
    const char* MacArrayToString(const uint8_t* arr, size_t _sizeMacString);


private:
    /**
     * @brief Background task for Wake-on-LAN execution.
     * @param parameter Task parameters.
     */
    static void runTaskWOL(void *parameter);

    /**
     * @brief Reads the HTTP response from the server.
     * @return The response as a string.
     */
    String ReadHTTPResponse();

    /**
     * @brief Sends an HTTP POST request.
     * @param reqPath The request path.
     * @param contentType The content type of the request.
     * @param data The data to send in the request body.
     * @return The server response as a string.
     */
    String HTTPPOST(const String &reqPath, const String &contentType, const String &data);

    /**
     * @brief Sends an HTTP GET request.
     * @param reqPath The request path.
     * @return The server response as a string.
     */
    String HTTPGET(const String &reqPath);

    /**
     * @brief Requests an OTA update from the server.
     * @param reqPath The request path for the update.
     * @return The status of the OTA request.
     */
    int RequestOTA(const String &reqPath);

    /**
     * @brief Updates the firmware with a given size.
     * @param FirmwareSize The size of the new firmware.
     * @return The update status.
     */
    int UpdateFirmware(int FirmwareSize);

   
private:
    TaskHandle_t handler; ///< Task handle for background tasks.
    int ServerPort;       ///< Server port number.
    IPAddress serverIP;   ///< Server IP address.
    IPAddress LocalIP;    ///< Local IP address.
    uint8_t MyMacAddr[6]; ///< Device MAC address.
    bool isOTAEnabled;    ///< Flag indicating if OTA updates are enabled.
    uint8_t WoLRepetion;  ///< Number of repetitions for Wake-on-LAN.

    /**
     * @brief Selects which client instance to use to perform the HTTP requests.
     *
     * If `OTA_WIFI_ETH == 1`, we are using WiFi client instance
     * else we are using the ethernet client instance
     */
#if (OTA_WIFI_ETH == 0)
    EthernetClient client; ///< Ethernet client instance.
#else
    WiFiClient client;       ///< Wi-Fi client instance.
    const char *ssid = "";   ///< Wi-Fi SSID.
    const char *passwd = ""; ///< Wi-Fi password.
#endif
};

#endif // NETWORK_STACK_H
