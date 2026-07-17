#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

typedef struct struct_message {
    int id;
    float temperature;
    float humidity;
    int batteryPercent;
} struct_message;

struct_message incomingData;
volatile bool newDataAvailable = false;
struct_message pendingData;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(struct_message)) return;
  memcpy(&pendingData, data, sizeof(pendingData));
  newDataAvailable = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (newDataAvailable) {
    newDataAvailable = false;
    incomingData = pendingData;

    // Print as JSON over serial — Pi will parse this line
    Serial.print("{\"id\":"); Serial.print(incomingData.id);
    Serial.print(",\"temperature\":"); Serial.print(incomingData.temperature, 2);
    Serial.print(",\"humidity\":"); Serial.print(incomingData.humidity, 2);
    Serial.print(",\"batteryPercent\":"); Serial.print(incomingData.batteryPercent);
    Serial.println("}");
  }
}