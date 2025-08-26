#include "ESP32_AP_STA.h"
/*
WebServer ESP32APserver(80);

const char *ap_ssid = "ESP32_AP";
const char *ap_password = "12345678";

String sta_ssid = "";
String sta_password = "";
String connectionStatus = "";
String token = "void";
String user = "void";


void initWiFiAPSTA()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Access Point started");
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  ESP32APserver.on("/", handleRoot);
  ESP32APserver.on("/connect", handleConnect);
  ESP32APserver.on("/status", handleStatus);
  ESP32APserver.on("/tokenUser", handleTokenUser);

  ESP32APserver.begin();
}

void handleRoot()
{
  String html = "<html><body><h2>WiFi Configuration</h2>";
  html += "<form action='/connect' method='GET'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='pass'><br>";
  html += "<input type='submit' value='Connect'>";
  html += "</form></body></html>";
  ESP32APserver.send(200, "text/html", html);
}

void handleConnect()
{
  sta_ssid = ESP32APserver.arg("ssid");
  sta_password = ESP32APserver.arg("pass");

  Serial.println("Received SSID: " + sta_ssid);
  Serial.println("Received Password: " + sta_password);

  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 50)
  {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConnected!");
    connectionStatus = "success";
  }
  else
  {
    Serial.println("\nFailed to connect");
    connectionStatus = "fail";
  }

  // Show HTML with delay before redirecting to /status
  String html = "<html><head>";
  //!!!!!!!!!!!!!!
  html += "<meta http-equiv='refresh' content='2;url=/status'>";
  html += "</head><body>";
  html += "<h3>Trying to connect to <strong>" + sta_ssid + "</strong>...</h3>";
  html += "<p>Please wait, checking connection status...</p>";
  html += "</body></html>";

  ESP32APserver.send(200, "text/html", html);
}

void handleStatus()
{
  if (connectionStatus == "success")
  {
    String html = "<h3>Connected to " + sta_ssid + " successfully!</h3>";
    html += "<br>Your ESP32 IP is: " + WiFi.localIP().toString();
    ESP32APserver.send(200, "text/html", html);

    // delay(2000);  // Wait 2 seconds before closing AP
    // WiFi.softAPdisconnect(true);
    // Serial.println("Access Point disabled.");
  }
  else if (connectionStatus == "fail")
  {
    ESP32APserver.send(200, "text/html", "<h3>Failed to connect. Try again.</h3>");
  }
  else
  {
    ESP32APserver.send(200, "text/html", "<h3>No connection attempt yet.</h3>");
  }
 // delay(2000); // Wait 10 seconds before closing AP
}
void handleTokenUser()
{
  String html = "<html><body><h2>Token and User Info</h2>";
  html += "<p>Token: " + token + "</p>";
  html += "<p>User: " + user + "</p>";
  html += "</body></html>";
  ESP32APserver.send(200, "text/html", html);
}

void handleWiFiServer()
{
  ESP32APserver.handleClient();
}
*/


/*
#include <WiFi.h>
#include <WebServer.h>

// Create web server
WebServer ESP32APserver(80);

// WiFi settings
const char* ap_ssid = "ESP32_AP";
const char* ap_password = "12345678";

String sta_ssid = "";
String sta_password = "";
String token = "void";
String user = "void";

// WiFi state machine
enum WiFiState {
  IDLE,
  CONNECTING,
  CONNECTED,
  FAILED
};

WiFiState wifiState = IDLE;
unsigned long connectStart = 0;
void processWiFiConnection() ;
void initWiFiAPSTA() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Access Point started");
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  ESP32APserver.on("/", handleRoot);
  ESP32APserver.on("/connect", handleConnect);
  ESP32APserver.on("/status", handleStatus);
  ESP32APserver.on("/tokenUser", handleTokenUser);

  ESP32APserver.begin();
}

// ---------- Handlers ----------

void handleRoot() {
  String html = "<html><body><h2>WiFi Configuration</h2>";
  html += "<form action='/connect' method='GET'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='pass'><br>";
  html += "<input type='submit' value='Connect'>";
  html += "</form></body></html>";
  ESP32APserver.send(200, "text/html", html);
}

void handleConnect() {
  sta_ssid = ESP32APserver.arg("ssid");
  sta_password = ESP32APserver.arg("pass");

  Serial.println("Received SSID: " + sta_ssid);
  Serial.println("Received Password: " + sta_password);

  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  wifiState = CONNECTING;
  connectStart = millis();

  // Redirect to /status
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='2;url=/status'>";
  html += "</head><body>";
  html += "<h3>Connecting to <strong>" + sta_ssid + "</strong>...</h3>";
  html += "<p>Please wait...</p>";
  html += "</body></html>";

  ESP32APserver.send(200, "text/html", html);
  delay(2000); // Short delay to allow status update
  handleStatus(); // Immediately check status
}

void handleStatus() {
  String html = "<html><head>";
  processWiFiConnection();

  if (wifiState == CONNECTED) {
    html += "<meta http-equiv='refresh' content='2;url=/tokenUser'>";
    html += "</head><body>";
    html += "<h3>✅ Connected to " + sta_ssid + " successfully!</h3>";
    html += "<p>Your ESP32 IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>Redirecting to token page...</p>";
  } 
  else if (wifiState == FAILED) {
    html += "</head><body><h3>❌ Failed to connect. Try again.</h3>";
  } 
  else {
    html += "<meta http-equiv='refresh' content='2;url=/status'>";
    html += "</head><body><h3>⌛ Still trying to connect...</h3>";
  }

  html += "</body></html>";
  ESP32APserver.send(200, "text/html", html);
  delay(2000); // Wait 2 seconds before next action
  handleTokenUser(); // Update token/user if connected
}

void handleTokenUser() {
  String html = "<html><body><h2>Token and User Info</h2>";
  html += "<p>Token: " + token + "</p>";
  html += "<p>User: " + user + "</p>";
  html += "<p>✅ Setup Finished</p>";
  html += "</body></html>";
  ESP32APserver.send(200, "text/html", html);
}

// ---------- Helpers ----------

void processWiFiConnection() {
  if (wifiState == CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      wifiState = CONNECTED;
    } 
    else if (millis() - connectStart > 15000) { // 15s timeout
      Serial.println("\nFailed to connect");
      wifiState = FAILED;
    }
  }
}

void handleWiFiServer() {
  ESP32APserver.handleClient();
}

*/
#include <WiFi.h>
#include <WebServer.h>

// Create web server
WebServer ESP32APserver(80);

// WiFi settings
const char* ap_ssid = "ESP32_AP";
const char* ap_password = "12345678";

String sta_ssid = "";
String sta_password = "";
String token = "void";
String user = "void";

// WiFi state machine
enum WiFiState {
  IDLE,
  CONNECTING,
  CONNECTED,
  FAILED
};

WiFiState wifiState = IDLE;
unsigned long connectStart = 0;

void processWiFiConnection() ;

void initWiFiAPSTA() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Access Point started");
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  ESP32APserver.on("/", handleRoot);
  ESP32APserver.on("/connect", handleConnect);
  ESP32APserver.on("/status", handleStatus);
  ESP32APserver.on("/tokenUser", handleTokenUser);

  ESP32APserver.begin();
}

// ---------- Handlers ----------

void handleRoot() {
  String html = "<html><body><h2>WiFi Configuration</h2>";
  html += "<form action='/connect' method='GET'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='pass'><br>";
  html += "<input type='submit' value='Connect'>";
  html += "</form></body></html>";
  ESP32APserver.send(200, "text/html", html);
}

void handleConnect() {
  sta_ssid = ESP32APserver.arg("ssid");
  sta_password = ESP32APserver.arg("pass");

  Serial.println("Received SSID: " + sta_ssid);
  Serial.println("Received Password: " + sta_password);

  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  wifiState = CONNECTING;
  connectStart = millis();

  // Redirect to /status
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='2;url=/status'>";
  html += "</head><body>";
  html += "<h3>Connecting to <strong>" + sta_ssid + "</strong>...</h3>";
  html += "<p>Please wait...</p>";
  html += "</body></html>";

  ESP32APserver.send(200, "text/html", html);
}

void handleStatus() {
  String html = "<html><head>";

  if (wifiState == CONNECTED) {
    html += "<meta http-equiv='refresh' content='2;url=/tokenUser'>";
    html += "</head><body>";
    html += "<h3>✅ Connected to " + sta_ssid + " successfully!</h3>";
    html += "<p>Your ESP32 IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>Redirecting to token page...</p>";
  } 
  else if (wifiState == FAILED) {
    html += "</head><body><h3>❌ Failed to connect. Try again.</h3>";
  } 
  else {
    html += "<meta http-equiv='refresh' content='2;url=/status'>";
    html += "</head><body><h3>⌛ Still trying to connect...</h3>";
  }

  html += "</body></html>";
  ESP32APserver.send(200, "text/html", html);
}

void handleTokenUser() {
  String html = "<html><body><h2>Token and User Info</h2>";
  html += "<p>Token: " + token + "</p>";
  html += "<p>User: " + user + "</p>";
  html += "<p>✅ Setup Finished</p>";
  html += "</body></html>";
  ESP32APserver.send(200, "text/html", html);
}

// ---------- Helpers ----------

void processWiFiConnection() {
  if (wifiState == CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      wifiState = CONNECTED;
    } 
    else if (millis() - connectStart > 15000) { // 15s timeout
      Serial.println("\nFailed to connect");
      wifiState = FAILED;
    }
  }
}

void handleWiFiServer() {
  ESP32APserver.handleClient();
}

// ---------- Arduino main ----------

// void setup() {
//   Serial.begin(115200);
//   initWiFiAPSTA();
// }

void loopWifiServer() {
  handleWiFiServer();
  processWiFiConnection();
}
