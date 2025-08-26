#include "OOP_ESP32_AP_STA.h"

OOP_ESP32_AP_STA::OOP_ESP32_AP_STA(const char* ap_ssid, const char* ap_password) 
    : _ap_ssid(ap_ssid), _ap_password(ap_password), ESP32APserver(80), 
      _wifiState(IDLE), _token("void"), _user("void") {
}

void OOP_ESP32_AP_STA::init() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_ap_ssid, _ap_password);

    Serial.println("Access Point started");
    Serial.println("AP IP address: " + WiFi.softAPIP().toString());

    _setupRoutes();
    ESP32APserver.begin();
}

void OOP_ESP32_AP_STA::_setupRoutes() {
    ESP32APserver.on("/", [this]() { _handleRoot(); });
    ESP32APserver.on("/connect", [this]() { _handleConnect(); });
    ESP32APserver.on("/status", [this]() { _handleStatus(); });
    ESP32APserver.on("/tokenUser", [this]() { _handleTokenUser(); });
    ESP32APserver.on("/done" , [this]() { _handleDone(); });
}

void OOP_ESP32_AP_STA::_handleRoot() {
    String html = R"(
        <html>
        <head>
            <title>ESP32 WiFi Configuration</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 40px; }
                form { background: #f0f0f0; padding: 20px; border-radius: 5px; }
                input[type="text"], input[type="password"] { 
                    width: 200px; padding: 8px; margin: 5px 0; 
                }
                input[type="submit"] { 
                    background: #007bff; color: white; padding: 10px 20px; 
                    border: none; border-radius: 3px; cursor: pointer; 
                }
            </style>
        </head>
        <body>
            <h2>WiFi Configuration</h2>
            <form action='/connect' method='GET'>
                SSID: <input type='text' name='ssid' required><br>
                Password: <input type='password' name='pass'><br>
                <input type='submit' value='Connect'>
            </form>
        </body>
        </html>
    )";
    ESP32APserver.send(200, "text/html", html);
}

void OOP_ESP32_AP_STA::_handleConnect() {
    _sta_ssid = ESP32APserver.arg("ssid");
    _sta_password = ESP32APserver.arg("pass");

    Serial.println("Received SSID: " + _sta_ssid);
    Serial.println("Received Password: " + _sta_password);

    WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str());
    _wifiState = CONNECTING;
    _connectStart = millis();

    String html = R"(
        <html>
        <head>
            <meta http-equiv='refresh' content='2;url=/status'>
            <title>Connecting...</title>
        </head>
        <body>
            <h3>Connecting to <strong>)" + _sta_ssid + R"(</strong>...</h3>
            <p>Please wait...</p>
        </body>
        </html>
    )";

    ESP32APserver.send(200, "text/html", html);
}

void OOP_ESP32_AP_STA::_handleStatus() {
    _processWiFiConnection();

    String html = "<html><head>";
    
    if (_wifiState == CONNECTED) {
        html += "<meta http-equiv='refresh' content='3;url=/tokenUser'>";
        html += "</head><body>";
        html += "<h3>✅ Connected to " + _sta_ssid + " successfully!</h3>";
        html += "<p>Your ESP32 IP: " + WiFi.localIP().toString() + "</p>";
        html += "<p>Redirecting to token page...</p>";
    } 
    else if (_wifiState == FAILED) {
        html += "</head><body><h3>❌ Failed to connect. Try again.</h3>";
        html += "<p><a href='/'>Go back</a></p>";
    } 
    else {
        html += "<meta http-equiv='refresh' content='2;url=/status'>";
        html += "</head><body><h3>⌛ Still trying to connect...</h3>";
    }

    html += "</body></html>";
    ESP32APserver.send(200, "text/html", html);
}

// Declare your global variable

//int tokenGeted = 0;


// Add a new handler in your server setup
void OOP_ESP32_AP_STA::_handleDone() {
    _tokenGeted = "done";
    ESP32APserver.send(200, "text/plain", "✅ Token received and saved. You can close this page.");
    Serial.println("Token received and saved.");
    Serial.println("Token: " + _token);
    Serial.println("User: " + _user);
}


void OOP_ESP32_AP_STA::_handleTokenUser() {
    String html = R"(
        <html>
        <head>
            <title>Token and User Info</title>
            <meta http-equiv="refresh" content="1">
            <style>
                body { font-family: Arial, sans-serif; margin: 40px; }
                .info { background: #e8f5e8; padding: 20px; border-radius: 5px; }
                button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
                .config-btn { background: #2196f3; color: white; }
                .done-btn { background: #4caf50; color: white; }
            </style>
        </head>
        <body>
            <h2>Token and User Info</h2>
            <div class='info'>
                <p><strong>Token:</strong> )" + _token + R"(</p>
                <p><strong>User:</strong> )" + _user + R"(</p>
            </div>
            <p>✅ Setup Finished</p>
            <form action="/" method="get">
                <button class="config-btn" type="submit">Configure Another Network</button>
            </form>
            <form action="/done" method="get">
                <button class="done-btn" type="submit">Done</button>
            </form>
        </body>
        </html>
    )";
    ESP32APserver.send(200, "text/html", html);
}


void OOP_ESP32_AP_STA::_processWiFiConnection() {
    if (_wifiState == CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected!");
            _wifiState = CONNECTED;
        } 
        else if (millis() - _connectStart > 15000) {
            Serial.println("\nFailed to connect");
            _wifiState = FAILED;
        }
    }
}

void OOP_ESP32_AP_STA::handleClient() {
    ESP32APserver.handleClient();
}

void OOP_ESP32_AP_STA::setToken(const String& newToken) {
    _token = newToken;
}

void OOP_ESP32_AP_STA::setUser(const String& newUser) {
    _user = newUser;
}

String OOP_ESP32_AP_STA::getConnectionStatus() const {
    switch (_wifiState) {
        case IDLE: return "IDLE";
        case CONNECTING: return "CONNECTING";
        case CONNECTED: return "CONNECTED";
        case FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

String OOP_ESP32_AP_STA::getStaSSID() const {
    return _sta_ssid;
}

IPAddress OOP_ESP32_AP_STA::getLocalIP() const {
    return WiFi.localIP();
}

bool OOP_ESP32_AP_STA::isConnected() const {
    return _wifiState == CONNECTED;
}

String OOP_ESP32_AP_STA::getTokenGeted() const {
    return _tokenGeted ;
}
