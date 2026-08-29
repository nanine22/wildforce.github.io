/* ESP32-C3 LoRa receiver and dashboard web server. */
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char* DEPT_NAME = "กรมอุทยานแห่งชาติ สัตว์ป่า และพันธุ์พืช";
const char* PERSON_NAME = "เจ้าหน้าที่อุทยาน01";
const char* AP_SSID = "WILDGUARD X";
const char* AP_PASSWORD = "12345678";
const char* CLOUD_API_URL = "https://data-jet-iota.vercel.app/api/data";
Preferences prefs;
WebServer server(80);
String savedSSID = "", savedPassword = "";
bool apMode = false, loraReady = false;

#define LED_PIN 0
#define LORA_CS 7
#define LORA_RST 2
#define LORA_DIO0 3
#define LORA_SCK 4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_FREQ 923E6
unsigned long ledOffAt = 0, lastUpdate = 0;
float spo2Value = 0, hrValue = 0, latValue = 0, lonValue = 0;
int statusValue = 0;
#define HISTORY_SIZE 20
float spo2History[HISTORY_SIZE], hrHistory[HISTORY_SIZE];
int historyIndex = 0, historyCount = 0;

void pushHistory(float spo2, float hr) {
  spo2History[historyIndex] = spo2;
  hrHistory[historyIndex] = hr;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) historyCount++;
}

String buildHistoryJson(float* values) {
  String json = "[";
  for (int i = 0; i < historyCount; i++) {
    int index = (historyIndex - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    json += String(values[index], 1);
    if (i < historyCount - 1) json += ",";
  }
  return json + "]";
}

void serveFile(const char* path, const char* contentType) {
  File file = LittleFS.open(path, "r");
  if (!file) { server.send(404, "text/plain", "File not found"); return; }
  server.streamFile(file, contentType);
  file.close();
}

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) { server.send(500, "text/plain", "index.html not found"); return; }
  String html = file.readString();
  file.close();
  html.replace("__DEPT_NAME__", String(DEPT_NAME));
  html.replace("__AP_BADGE__", apMode ? "<p class=\"badge\">Waiting to Connect Wi-Fi</p>" : "");
  html.replace("__CURRENT_SSID__", savedSSID.length() ? "<p class=\"current\">Connected Wi-Fi: " + savedSSID + "</p>" : "");
  server.send(200, "text/html; charset=utf-8", html);
}

void handleWifiSave() {
  String newSSID = server.arg("ssid"), newPassword = server.arg("password");
  if (!newSSID.length()) { server.send(400, "text/html; charset=utf-8", "<p>กรุณากรอกชื่อ WiFi</p>"); return; }
  prefs.begin("wifi", false);
  prefs.putString("ssid", newSSID);
  prefs.putString("password", newPassword);
  prefs.end();
  server.send(200, "text/html; charset=utf-8", "<!DOCTYPE html><html lang=\"th\"><head><meta charset=\"UTF-8\"><link rel=\"stylesheet\" href=\"/style.css\"></head><body><div><h1>บันทึกสำเร็จ</h1><p>บอร์ดกำลังรีสตาร์ทเพื่อเชื่อมต่อ WiFi ใหม่...</p></div></body></html>");
  delay(1500);
  ESP.restart();
}

void handleApiData() {
  bool hasData = lastUpdate != 0;
  unsigned long ageSec = hasData ? (millis() - lastUpdate) / 1000 : 0;
  String json = "{\"name\":\"" + String(PERSON_NAME) + "\",\"spo2\":" + String(spo2Value, 1);
  json += ",\"hr\":" + String(hrValue, 1) + ",\"status\":" + String(statusValue);
  json += ",\"lat\":" + String(latValue, 6) + ",\"lon\":" + String(lonValue, 6);
  json += ",\"hasData\":" + String(hasData ? "true" : "false") + ",\"ageSec\":" + String(ageSec);
  json += ",\"spo2History\":" + buildHistoryJson(spo2History) + ",\"hrHistory\":" + buildHistoryJson(hrHistory) + "}";
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json", json);
}

void publishToCloud() {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, CLOUD_API_URL)) return;
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"name\":\"" + String(PERSON_NAME) + "\",\"spo2\":" + String(spo2Value, 1);
  payload += ",\"hr\":" + String(hrValue, 1) + ",\"status\":" + String(statusValue);
  payload += ",\"lat\":" + String(latValue, 6) + ",\"lon\":" + String(lonValue, 6) + "}";
  int responseCode = http.POST(payload);
  if (responseCode < 200 || responseCode >= 300) {
    Serial.print("Cloud upload failed: ");
    Serial.println(responseCode);
  }
  http.end();
}

void loadWifiCredentials() {
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPassword = prefs.getString("password", "");
  prefs.end();
}

bool connectSavedWifi() {
  if (!savedSSID.length()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) { delay(500); Serial.print("."); }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void startAPMode() {
  apMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed");
  loadWifiCredentials();
  if (!connectSavedWifi()) startAPMode();
  server.on("/", handleRoot);
  server.on("/wifi", []() { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });
  server.on("/wifi/save", HTTP_POST, handleWifiSave);
  server.on("/data", []() { serveFile("/dashboard.html", "text/html; charset=utf-8"); });
  server.on("/preview.html", []() { serveFile("/preview.html", "text/html; charset=utf-8"); });
  server.on("/style.css", []() { serveFile("/style.css", "text/css"); });
  server.on("/dashboard.js", []() { serveFile("/dashboard.js", "application/javascript"); });
  server.on("/image.png", []() { serveFile("/image.png", "image/png"); });
  server.on("/api/data", handleApiData);
  server.begin();
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  loraReady = LoRa.begin(LORA_FREQ);
  if (loraReady) { LoRa.setSyncWord(0xF3); LoRa.setSpreadingFactor(11); LoRa.setSignalBandwidth(125E3); }
}

void loop() {
  server.handleClient();
  if (ledOffAt && millis() >= ledOffAt) { digitalWrite(LED_PIN, LOW); ledOffAt = 0; }
  if (!loraReady || !LoRa.parsePacket()) return;
  String received = "";
  while (LoRa.available()) received += (char)LoRa.read();
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, received);
  if (error) { Serial.print("JSON parse error: "); Serial.println(error.c_str()); return; }
  digitalWrite(LED_PIN, HIGH);
  ledOffAt = millis() + 150;
  spo2Value = doc["spo2"] | spo2Value;
  hrValue = doc["hr"] | hrValue;
  statusValue = doc["status"] | statusValue;
  latValue = doc["lat"] | latValue;
  lonValue = doc["lon"] | lonValue;
  lastUpdate = millis();
  pushHistory(spo2Value, hrValue);
  publishToCloud();
}
