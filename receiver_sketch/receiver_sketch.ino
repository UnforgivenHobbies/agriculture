#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_now.h>

// ===== CONFIG =====
const char* ssid = "Sociality.gr";
const char* password = "valekatiaplo";
const char* mqttServer = "192.168.1.30";
const int mqttPort = 1883;

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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
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