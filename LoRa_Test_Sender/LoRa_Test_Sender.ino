/*
  LoRa test sender for the Wildforce ESP32-C3 receiver.
  Sends the same JSON fields expected by the receiver.
*/

#include <SPI.h>
#include <LoRa.h>

#define LORA_CS 7
#define LORA_RST 2
#define LORA_DIO0 3
#define LORA_SCK 4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_FREQ 923E6

const unsigned long SEND_INTERVAL_MS = 2000;
unsigned long lastSendAt = 0;
unsigned long packetNumber = 0;

// Test scenarios: normal, low SpO2, low heart rate, and fall.
struct TestData {
  float spo2;
  float hr;
  int status;
  float lat;
  float lon;
};

const TestData scenarios[] = {
  {98.0, 72.0, 0, 13.756300, 100.501800},
  {94.5, 84.0, 0, 13.756450, 100.501950},
  {88.0, 76.0, 0, 13.756600, 100.502100},
  {96.0, 45.0, 0, 13.756750, 100.502250},
  {97.0, 78.0, 1, 13.756900, 100.502400},
  {97.0, 78.0, 2, 13.757050, 100.502550},
  {97.0, 78.0, 3, 13.757200, 100.502700}
};

const size_t SCENARIO_COUNT = sizeof(scenarios) / sizeof(scenarios[0]);
size_t scenarioIndex = 0;

void sendTestData() {
  const TestData& data = scenarios[scenarioIndex];
  String payload = "{";
  payload += "\"spo2\":" + String(data.spo2, 1) + ",";
  payload += "\"hr\":" + String(data.hr, 1) + ",";
  payload += "\"status\":" + String(data.status) + ",";
  payload += "\"lat\":" + String(data.lat, 6) + ",";
  payload += "\"lon\":" + String(data.lon, 6);
  payload += "}";

  LoRa.beginPacket();
  LoRa.print(payload);
  int result = LoRa.endPacket();

  Serial.print("Packet #");
  Serial.print(packetNumber++);
  Serial.print(" | ");
  Serial.print(payload);
  Serial.print(" | result=");
  Serial.println(result == 1 ? "sent" : "failed");

  scenarioIndex = (scenarioIndex + 1) % SCENARIO_COUNT;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed. Check wiring and frequency.");
    while (true) delay(1000);
  }

  LoRa.setSyncWord(0xF3);
  LoRa.setSpreadingFactor(11);
  LoRa.setSignalBandwidth(125E3);
  Serial.println("LoRa test sender ready");
}

void loop() {
  if (millis() - lastSendAt < SEND_INTERVAL_MS) return;
  lastSendAt = millis();
  sendTestData();
}
