#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include "esp_wifi.h"

// ==== CONFIG ====
#define I2C_SDA 21
#define I2C_SCL 22
#define BAT_PIN 0

#define NORMAL_SLEEP_MIN 30      // 30 minutes
#define LOWBAT_SLEEP_MIN 120     // 2 hours
#define uS_TO_S_FACTOR 1000000ULL

#define MAX_SEND_ATTEMPTS 5
#define POST_SEND_DELAY_MS 15000  // 15 seconds delay before sleep

uint8_t receiverMac[] = {0xF0, 0xF5, 0xBD, 0x2C, 0x0F, 0x68};

// ==== SENSOR ====
Adafruit_SHT31 sht30 = Adafruit_SHT31();

// ==== DATA STRUCT ====
typedef struct struct_message {
  int id;
  float temperature;
  float humidity;
  int batteryPercent;
} struct_message;

struct_message myData;

// ==== BATTERY FUNCTION ====
int readBatteryPercentage(float &batteryVolts) {
  long sum = 0;
  for (int i = 0; i < 10; i++) sum += analogReadMilliVolts(BAT_PIN);
  int analogMilliVolts = sum / 10;
  batteryVolts = (analogMilliVolts * 2) / 1000.0;

  int percent = (int)(((batteryVolts - 3.0) / (4.2 - 3.0)) * 100.0);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;
}

// ==== SETUP ====
void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!sht30.begin(0x44)) Serial.println("Couldn't find SHT30 sensor");

  // Wi-Fi station mode
  WiFi.mode(WIFI_STA);

  // Long-range ESP-NOW (sender only)
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  esp_wifi_set_max_tx_power(78);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  // Log send status
  esp_now_register_send_cb([](const esp_now_send_info_t *info, esp_now_send_status_t status){
    Serial.print("ESP-NOW send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  });

  // Add receiver as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) Serial.println("Failed to add peer");

  // ==== READ SENSORS ====
  float batteryVolts;
  int batteryPercent = readBatteryPercentage(batteryVolts);
  float temp = sht30.readTemperature();
  float hum = sht30.readHumidity();

  myData.id = 1;
  myData.temperature = isnan(temp) ? -99 : temp;
  myData.humidity = isnan(hum) ? -99 : hum;
  myData.batteryPercent = batteryPercent;

  Serial.printf("Battery: %.2fV (%d%%), Temp: %.2f°C, Hum: %.2f%%\n",
                batteryVolts, batteryPercent, myData.temperature, myData.humidity);

  // ==== TRY TO SEND ====
  bool sent = false;
  int attempts = 0;
  while (!sent && attempts < MAX_SEND_ATTEMPTS) {
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Send succeeded");
      sent = true;
    } else {
      attempts++;
      Serial.printf("Send failed, attempt %d/%d → retrying in 2 seconds\n", attempts, MAX_SEND_ATTEMPTS);
      delay(2000);
    }
  }

  if (!sent) Serial.println("Failed to send after max attempts, going to sleep anyway");

  // ==== POST SEND DELAY ====
  Serial.printf("Waiting %d seconds before sleep...\n", POST_SEND_DELAY_MS / 1000);
  delay(POST_SEND_DELAY_MS);

  // ==== DEEP SLEEP ====
  uint64_t sleepTimeMin = (batteryPercent < 20) ? LOWBAT_SLEEP_MIN : NORMAL_SLEEP_MIN;
  Serial.printf("Sleeping for %llu minutes...\n", sleepTimeMin);
  esp_sleep_enable_timer_wakeup(sleepTimeMin * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // never runs
}