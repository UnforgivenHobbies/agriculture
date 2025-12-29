#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_now.h>
// Import wifi ssid, wifi password, mqtt server ip and mqtt server port
#include "credentials.h"

// ===== CONFIG =====y
WiFiClient espClient;
PubSubClient client(espClient);

// ===== DATA STRUCT =====
typedef struct struct_message {
    int id;
    float temperature;
    float humidity;
    int batteryPercent;
} struct_message;

struct_message incomingData;

// ===== WIFI + MQTT =====
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi channel: ");
  Serial.println(WiFi.channel());

}

void connectToMQTT() {
  if (client.connected()) return;

  if (WiFi.status() != WL_CONNECTED) connectToWiFi();

  Serial.println("Connecting to MQTT...");
  if (client.connect("ESP32Receiver")) {
    Serial.println("Connected to MQTT!");
  } else {
    Serial.print("MQTT connect failed, rc=");
    Serial.println(client.state());
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  Serial.println(message);
}

// ===== ESP-NOW RECEIVE =====
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(struct_message)) return;

  memcpy(&incomingData, data, sizeof(incomingData));

  Serial.println("--- ESP-NOW Data ---");
  Serial.printf("ID: %d\n", incomingData.id);
  Serial.printf("Temp: %.2f °C\n", incomingData.temperature);
  Serial.printf("Hum: %.2f %%\n", incomingData.humidity);
  Serial.printf("Battery: %d %%\n", incomingData.batteryPercent);

  if (client.connected()) {
    String payload = "{";
    payload += "\"id\":" + String(incomingData.id) + ",";
    payload += "\"temperature\":" + String(incomingData.temperature, 2) + ",";
    payload += "\"humidity\":" + String(incomingData.humidity, 2) + ",";
    payload += "\"batteryPercent\":" + String(incomingData.batteryPercent);
    payload += "}";
    client.publish("garden/sensor1", payload.c_str());
    Serial.println("Published to MQTT: " + payload);
  } else {
    Serial.println("MQTT not connected");
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);

  connectToWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  connectToMQTT();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(OnDataRecv);
}

// ===== LOOP =====
void loop() {
  connectToMQTT();
  client.loop();
}
