#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// ===== USER CODE START =====
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

/* ============================
   USER CONFIG (FILL LATER)
   ============================ */

const char* WIFI_SSID = "GP";
const char* WIFI_PASSWORD = "gagancn2005&";
const char* BACKEND_BASE_URL = "http://192.168.56.1:8000";
const char* DEVICE_ID = "esp32_01";

#define STATUS_LED_PIN 2

const unsigned long TELEMETRY_INTERVAL = 10000;
const unsigned long OTA_POLL_INTERVAL = 15000;

unsigned long lastTelemetry = 0;
unsigned long lastOtaPoll = 0;
bool ledState = false;

void ledOn() { digitalWrite(STATUS_LED_PIN, HIGH); }
void ledOff() { digitalWrite(STATUS_LED_PIN, LOW); }
void ledToggle() { ledState = !ledState; digitalWrite(STATUS_LED_PIN, ledState); }

void connectToWiFi() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  ledOff();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    ledToggle();
    delay(500);
  }
  ledOn();
}

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;
  StaticJsonDocument<512> doc;
  JsonObject pins = doc.createNestedObject("pins");
  for (int pin = 0; pin <= 39; pin++) {
    if (pin == STATUS_LED_PIN) continue;
    pinMode(pin, INPUT);
    pins[String(pin)] = digitalRead(pin);
  }
  String payload;
  serializeJson(doc, payload);
  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/telemetry/" + DEVICE_ID;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();
}

void checkForOTA() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/check?device_id=" + DEVICE_ID;
  http.begin(url);
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  String response = http.getString();
  http.end();
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, response)) return;
  if (!doc["update"]) return;
  performOTA(doc["build_id"]);
}

void performOTA(const char* buildId) {
  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/download/" + buildId;
  http.begin(url);
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  int len = http.getSize();
  WiFiClient* stream = http.getStreamPtr();
  if (!Update.begin(len)) { http.end(); return; }
  Update.writeStream(*stream);
  if (Update.end()) ESP.restart();
  http.end();
}

void setup() {
  Serial.begin(9600);
  connectToWiFi();
}

void loop() {
  unsigned long now = millis();
  if (now - lastTelemetry > TELEMETRY_INTERVAL) {
    lastTelemetry = now;
    sendTelemetry();
  }
  if (now - lastOtaPoll > OTA_POLL_INTERVAL) {
    lastOtaPoll = now;
    checkForOTA();
  }
}
// ===== USER CODE END =====

void setup() {
  Serial.begin(115200);
  user_setup();
}

void loop() {
  user_loop();
}
