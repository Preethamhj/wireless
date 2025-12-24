#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

/* =====================================================
   CONFIGURATION
   ===================================================== */

const char* WIFI_SSID     = "vivo#1933#97";
const char* WIFI_PASSWORD = "29911114";

const char* BACKEND_BASE_URL = "http://192.168.210.116:8000";
const char* DEVICE_ID = "esp32_01";

#define STATUS_LED_PIN 2

/* =====================================================
   SAFE GPIO LIST
   ===================================================== */

const int SAFE_PINS[] = {
  0, 2, 4, 5,
  12, 13, 14, 15,
  16, 17, 18, 19,
  21, 22, 23,
  25, 26, 27,
  32, 33,
  34, 35, 36, 39
};
const int SAFE_PIN_COUNT = sizeof(SAFE_PINS) / sizeof(SAFE_PINS[0]);

/* =====================================================
   TIMING
   ===================================================== */

unsigned long lastOtaCheck    = 0;
unsigned long lastTelemetry  = 0;
unsigned long lastLedBlink   = 0;
unsigned long lastWifiRetry  = 0;

const unsigned long OTA_INTERVAL       = 10000;
const unsigned long TELEMETRY_INTERVAL = 20000;
const unsigned long LED_BLINK_INTERVAL = 5000;
const unsigned long WIFI_RETRY_DELAY   = 15000;

/* =====================================================
   OTA STATE
   ===================================================== */

bool otaInProgress = false;
bool otaCompleted  = false;
bool ledState      = false;

/* =====================================================
   WIFI
   ===================================================== */

void connectWiFi() {
  Serial.println("\n========== WIFI ==========");
  Serial.printf("[WIFI] Connecting to %s\n", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\n[WIFI] Timeout (retry later)");
      return;
    }
  }

  Serial.println("\n[WIFI] Connected");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
}

/* =====================================================
   OTA EVENT
   ===================================================== */

void notifyFallbackEvent(String buildId, String status, String reason) {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String(BACKEND_BASE_URL) + "/ota/fallback-event";
  Serial.println("\n[API] POST " + url);

  HTTPClient http;

  StaticJsonDocument<256> doc;
  doc["device_id"] = DEVICE_ID;
  doc["build_id"]  = buildId;
  doc["status"]    = status;
  doc["reason"]    = reason;

  String payload;
  serializeJson(doc, payload);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String resp = http.getString();

  Serial.printf("[API] HTTP %d\n", code);
  Serial.println("[API] Response:");
  Serial.println(resp);

  http.end();
}

/* =====================================================
   OTA PUSH (FALLBACK)
   ===================================================== */

bool performFallbackOTAPush(String buildId) {
  Serial.println("\n========== OTA PUSH ==========");

  notifyFallbackEvent(buildId, "started", "pull_failed");

  String url = String(BACKEND_BASE_URL) + "/ota/push/" + DEVICE_ID;
  Serial.println("[API] POST " + url);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST("{}");
  Serial.printf("[API] HTTP %d\n", code);

  if (code != HTTP_CODE_OK) {
    Serial.println("[OTA-PUSH] Request failed");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();

  if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
    Serial.println("[OTA-PUSH] Update.begin failed");
    http.end();
    return false;
  }

  Serial.println("[OTA-PUSH] Writing firmware...");
  Update.writeStream(*stream);

  if (!Update.end()) {
    Serial.println("[OTA-PUSH] Flash failed");
    http.end();
    return false;
  }

  notifyFallbackEvent(buildId, "success", "push_success");
  http.end();
  return true;
}

/* =====================================================
   OTA PULL
   ===================================================== */

void performOTA(String buildId) {
  Serial.println("\n========== OTA PULL ==========");

  String url = String(BACKEND_BASE_URL) + "/ota/download/" + buildId;
  Serial.println("[API] GET " + url);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();

  Serial.printf("[API] HTTP %d\n", code);

  if (code != HTTP_CODE_OK) {
    http.end();
    if (performFallbackOTAPush(buildId)) {
      delay(2000);
      ESP.restart();
    }
    otaInProgress = false;
    return;
  }

  WiFiClient* stream = http.getStreamPtr();

  if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
    Serial.println("[OTA] Update.begin failed");
    http.end();
    otaInProgress = false;
    return;
  }

  Serial.println("[OTA] Writing firmware...");
  Update.writeStream(*stream);

  if (!Update.end()) {
    Serial.println("[OTA] Pull failed → fallback");
    http.end();
    if (performFallbackOTAPush(buildId)) {
      delay(2000);
      ESP.restart();
    }
    otaInProgress = false;
    return;
  }

  Serial.println("[OTA] SUCCESS → rebooting");
  delay(2000);
  ESP.restart();
}

/* =====================================================
   OTA CHECK
   ===================================================== */

void checkOTA() {
  if (otaInProgress || otaCompleted) return;
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String(BACKEND_BASE_URL) + "/ota/check?device_id=" + DEVICE_ID;
  Serial.println("\n[API] GET " + url);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  String resp = http.getString();

  Serial.printf("[API] HTTP %d\n", code);
  Serial.println("[API] Response:");
  Serial.println(resp);

  if (code != HTTP_CODE_OK) {
    http.end();
    return;
  }

  StaticJsonDocument<256> doc;
  deserializeJson(doc, resp);

  if (!doc["update"]) {
    Serial.println("[OTA] No update available");
    http.end();
    return;
  }

  String buildId = doc["build_id"].as<String>();
  Serial.printf("[OTA] Update FOUND → %s\n", buildId.c_str());

  otaInProgress = true;
  http.end();
  performOTA(buildId);
}

/* =====================================================
   TELEMETRY
   ===================================================== */

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String(BACKEND_BASE_URL) + "/telemetry/" + DEVICE_ID;
  Serial.println("\n[API] POST " + url);

  StaticJsonDocument<1024> doc;
  doc["device_id"] = DEVICE_ID;
  JsonObject pins = doc.createNestedObject("pins");

  for (int i = 0; i < SAFE_PIN_COUNT; i++) {
    pins[String(SAFE_PINS[i])] = digitalRead(SAFE_PINS[i]);
  }

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String resp = http.getString();

  Serial.printf("[API] HTTP %d\n", code);
  Serial.println("[API] Response:");
  Serial.println(resp);

  http.end();
}

/* =====================================================
   LED
   ===================================================== */

void blinkLed() {
  if (millis() - lastLedBlink >= LED_BLINK_INTERVAL) {
    lastLedBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState);
  }
}

/* =====================================================
   SETUP & LOOP
   ===================================================== */

void setup() {
  Serial.begin(9600);
  pinMode(STATUS_LED_PIN, OUTPUT);

  for (int i = 0; i < SAFE_PIN_COUNT; i++) {
    pinMode(SAFE_PINS[i], INPUT);
  }

  connectWiFi();
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED &&
      now - lastWifiRetry > WIFI_RETRY_DELAY) {
    lastWifiRetry = now;
    connectWiFi();
  }

  if (now - lastOtaCheck >= OTA_INTERVAL) {
    lastOtaCheck = now;
    checkOTA();
  }

  if (now - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = now;
    sendTelemetry();
  }

  blinkLed();
  delay(10);
}
