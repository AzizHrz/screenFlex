#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

const char* device_id = "esp32-01";
String SCHEDULE_TOPIC = "screenflex/" + String(device_id) + "/schedule";
String ORDER_TOPIC = "screenflex/" + String(device_id) + "/order";
String STATE_TOPIC = "screenflex/" + String(device_id) + "/state";

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void handleSchedule(JsonObject doc) {
  JsonObject schedule = doc["schedule"];
  JsonArray days = schedule["days"];

  for (int i = 0; i < days.size(); i++) {
    JsonObject day = days[i];
    bool enabled = day["enabled"];
    Serial.printf("Day %d - Enabled: %s\n", i, enabled ? "true" : "false");

    JsonArray slots = day["slots"];
    if (slots.isNull() || slots.size() == 0) {
      Serial.println("  No slots for this day");
      continue;
    }

    for (JsonObject slot : slots) {
      const char* start = slot["start_time"];
      const char* end = slot["end_time"];
      Serial.printf("  Slot: %s -> %s\n", start, end);
    }
  }
}

void handleOrder(JsonObject doc) {
  Serial.println("Received order mode = schedule");
  // Add custom logic here for order messages
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)message[i];
  }

  Serial.println("Message: " + msg);

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  const char* mode = doc["mode"];

  String topicStr = String(topic);
  if (topicStr.endsWith("/schedule")) {
    if (strcmp(mode, "schedule") == 0) {
      handleSchedule(doc);
    }
  } else if (topicStr.endsWith("/order")) {
    if (strcmp(mode, "schedule") == 0) {
      handleOrder(doc);
    }
  }

  // Simulate current TV state
  StaticJsonDocument<200> stateDoc;
  stateDoc["currentTVState"] = 0; // TV_ON
  char buffer[256];
  serializeJson(stateDoc, buffer);
  client.publish(STATE_TOPIC.c_str(), buffer);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(SCHEDULE_TOPIC.c_str());
      client.subscribe(ORDER_TOPIC.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
