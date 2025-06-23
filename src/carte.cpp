#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* ssid = "OPPO Reno12 F";
const char* password = "h4qu37mv";

// Server Info
IPAddress serverIp(192, 168, 1, 9); // ← Your Flask server IP
const int serverPort = 5000;

// Device ID (Use an existing device ID from server)
String DEVICE_ID = "387140789AE1114B613906A6"; // ← Change if needed

unsigned long lastPingTime = 0;

bool connectToNetwork() {
    if (WiFi.status() == WL_CONNECTED) return true;

    WiFi.begin(ssid, password);
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(500);
        Serial.print(".");
    }

    return WiFi.status() == WL_CONNECTED;
}

void testPingEndpoint() {
    if (!connectToNetwork()) {
        Serial.println("[PING] Failed: No network connection");
        return;
    }

    HTTPClient http;
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/ping";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Simulate current time
    String simulatedTime = "2024-01-01T12:00:00Z";

    DynamicJsonDocument doc(1024);
    doc["actual_state"] = "on"; // Simulate current TV state
    doc["desired_state"] = "off"; // Desired TV state
    doc["current_time"] = simulatedTime;

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("[PING] Response: %s\n", response.c_str());
    } else {
        Serial.printf("[PING] Failed with code %d\n", httpCode);
    }
    http.end();
}

void testModeEndpoints() {
    // GET mode
    {
        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/mode";
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("[MODE] GET Response: %s\n", payload.c_str());
        } else {
            Serial.printf("[MODE] GET failed with code %d\n", httpCode);
        }
        http.end();
    }

    // POST mode
    {
        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/mode";
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String postData = "{\"use_schedule\":false}";
        int httpCode = http.POST(postData);
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("[MODE] POST Response: %s\n", payload.c_str());
        } else {
            Serial.printf("[MODE] POST failed with code %d\n", httpCode);
        }
        http.end();
    }
}

void testTVEndpoints() {
    // GET TV state
    {
        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/tv";
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("[TV] GET Response: %s\n", payload.c_str());
        } else {
            Serial.printf("[TV] GET failed with code %d\n", httpCode);
        }
        http.end();
    }

    // POST TV command
    {
        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/tv";
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String postData = "{\"status\":\"on\"}";
        int httpCode = http.POST(postData);
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("[TV] POST Response: %s\n", payload.c_str());
        } else {
            Serial.printf("[TV] POST failed with code %d\n", httpCode);
        }
        http.end();
    }
}

void testScheduleEndpoints() {
    // GET Schedule
    {
        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/schedule";
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("[SCHEDULE] GET Response: %s\n", payload.c_str());
        } else {
            Serial.printf("[SCHEDULE] GET failed with code %d\n", httpCode);
        }
        http.end();
    }

    // POST Schedule
    {
        DynamicJsonDocument doc(1024);
        doc["active"] = true;
        doc["timezone"] = "UTC+1";
        doc["start_time"] = "09:00";
        doc["end_time"] = "21:00";
        JsonArray weekdays = doc.createNestedArray("weekdays");
        weekdays.add("Mon");
        weekdays.add("Wed");
        weekdays.add("Fri");

        String payload;
        serializeJson(doc, payload);

        HTTPClient http;
        String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/schedule";
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        int httpCode = http.POST(payload);
        if (httpCode == HTTP_CODE_OK) {
            String response = http.getString();
            Serial.printf("[SCHEDULE] POST Response: %s\n", response.c_str());
        } else {
            Serial.printf("[SCHEDULE] POST failed with code %d\n", httpCode);
        }
        http.end();
    }
}

void testReportState() {
    HTTPClient http;
    String url = "http://" + serverIp.toString() + ":" + String(serverPort) + "/esp/" + DEVICE_ID + "/state";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String postData = "{\"actual_state\":\"on\"}";

    int httpCode = http.POST(postData);
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.printf("[STATE] Response: %s\n", payload.c_str());
    } else {
        Serial.printf("[STATE] Failed with code %d\n", httpCode);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");

    testPingEndpoint();
    testModeEndpoints();
    testTVEndpoints();
    testScheduleEndpoints();
    testReportState();

    lastPingTime = millis();
}

void loop() {
    if (millis() - lastPingTime > 30000) {
        Serial.println("\n\n--- Running Periodic Tests ---");
        testPingEndpoint();
        testModeEndpoints();
        testTVEndpoints();
        testScheduleEndpoints();
        testReportState();
        lastPingTime = millis();
    }

    delay(1000);
}