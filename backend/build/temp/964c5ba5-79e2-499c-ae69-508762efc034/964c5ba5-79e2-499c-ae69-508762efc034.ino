
/*************************************************
 * ESP32 OTA + Digital Twin Firmware
 *************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* BACKEND_BASE_URL = "http://YOUR_BACKEND_IP:8000";
const char* DEVICE_ID = "esp32_01";

#define STATUS_LED_PIN 2

unsigned long lastTelemetry = 0;
unsigned long lastOtaPoll = 0;

/* ===== USER CODE START ===== */

void userSetup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
}

void userLoop() {
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(1000);
}

/* ===== USER CODE END ===== */

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;

  StaticJsonDocument<256> doc;
  JsonObject pins = doc.createNestedObject("pins");

  for (int pin = 0; pin <= 39; pin++) {
    pinMode(pin, INPUT);
    pins[String(pin)] = digitalRead(pin);
  }

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(String(BACKEND_BASE_URL) + "/telemetry/" + DEVICE_ID);
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();
}

void checkOTA() {
  HTTPClient http;
  http.begin(String(BACKEND_BASE_URL) + "/ota/check?device_id=" + String(DEVICE_ID));
  if (http.GET() != 200) {
    http.end();
    return;
  }

  StaticJsonDocument<128> doc;
  deserializeJson(doc, http.getString());
  http.end();

  if (!doc["update"]) return;

  String buildId = doc["build_id"].as<String>();

  http.begin(String(BACKEND_BASE_URL) + "/ota/download/" + buildId);
  if (http.GET() != 200) {
    http.end();
    return;
  }

  int size = http.getSize();
  WiFiClient* stream = http.getStreamPtr();

  if (Update.begin(size)) {
    Update.writeStream(*stream);
    if (Update.end()) ESP.restart();
  }
  http.end();
}

void setup() {
  Serial.begin(9600);
  connectWiFi();
  userSetup();
}

void loop() {
  Serial.print(" updated code");
  unsigned long now = millis();

  if (now - lastTelemetry > 10000) {
    lastTelemetry = now;
    sendTelemetry();
  }

  if (now - lastOtaPoll > 15000) {
    lastOtaPoll = now;
    checkOTA();
  }

  userLoop();
}
