#ifndef ESP32_AP_STA_H
#define ESP32_AP_STA_H

#include <WiFi.h>
#include <WebServer.h>

class OOP_ESP32_AP_STA {
public:
    // Constructor
    OOP_ESP32_AP_STA(const char* ap_ssid = "ESP32_AP", const char* ap_password = "12345678");
    
    // Initialization
    void init();
    void handleClient();
    
    // Setters for token and user
    void setToken(const String& newToken);
    void setUser(const String& newUser);
    
    // Getters
    String getConnectionStatus() const;
    String getStaSSID() const;
    IPAddress getLocalIP() const;
    bool isConnected() const;

    String getTokenGeted() const ;

private:
    // WiFi configuration
    const char* _ap_ssid;
    const char* _ap_password;
    String _sta_ssid;
    String _sta_password;
    
    // Server and state
    WebServer ESP32APserver;
    enum WiFiState { IDLE, CONNECTING, CONNECTED, FAILED };
    WiFiState _wifiState;
    unsigned long _connectStart;
    
    // User data
    String _token;
    String _user;
    String _tokenGeted;

    
    // Private methods
    void _setupRoutes();
    void _processWiFiConnection();
    
    // Route handlers
    void _handleRoot();
    void _handleConnect();
    void _handleStatus();
    void _handleDone() ;
    void _handleTokenUser();

};

#endif // ESP32_AP_STA_H