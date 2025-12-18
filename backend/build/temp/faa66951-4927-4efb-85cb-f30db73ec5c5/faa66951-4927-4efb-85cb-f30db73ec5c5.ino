#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

/* =====================================================
   CONFIGURATION
   ===================================================== */

const char* WIFI_SSID     = "POWER 2049";
const char* WIFI_PASSWORD = "123456789@#";

const char* BACKEND_BASE_URL = "http://192.168.137.1:8000";
const char* DEVICE_ID = "esp32_01";

#define STATUS_LED_PIN 2

/* =====================================================
   TIMING CONFIG
   ===================================================== */

unsigned long lastOtaCheck = 0;
unsigned long lastTelemetry = 0;
unsigned long lastLedBlink = 0;

const unsigned long OTA_INTERVAL = 5000;
const unsigned long TELEMETRY_INTERVAL = 10000;
const unsigned long LED_BLINK_INTERVAL = 5000;

/* =====================================================
   OTA STATE FLAGS
   ===================================================== */

bool otaInProgress = false;
bool otaCompleted  = false;
bool ledState      = false;

/* =====================================================
   WIFI CONNECT
   ===================================================== */

void connectWiFi() {
  Serial.println("\n========== WIFI ==========");
  Serial.print("[WIFI] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\n[WIFI] Timeout → rebooting");
      ESP.restart();
    }
  }

  Serial.println("\n[WIFI] Connected");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
}

/* =====================================================
   🆕 BACKEND FALLBACK NOTIFY
   ===================================================== */

void notifyFallbackEvent(String buildId, String status, String reason) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/fallback-event";

  StaticJsonDocument<256> doc;
  doc["device_id"] = DEVICE_ID;
  doc["build_id"]  = buildId;
  doc["status"]    = status;   // started | success | failed
  doc["reason"]    = reason;

  String payload;
  serializeJson(doc, payload);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);

  Serial.print("[OTA-FB] Notify backend → HTTP ");
  Serial.println(code);

  http.end();
}

/* =====================================================
   FALLBACK OTA (PUSH MODE)
   ===================================================== */

bool performFallbackOTAPush(String buildId) {
  Serial.println("\n========== OTA FALLBACK (PUSH) ==========");

  notifyFallbackEvent(buildId, "started", "pull_ota_failed");

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/push/" + DEVICE_ID;

  Serial.print("[OTA-FB] POST ");
  Serial.println(url);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST("{}");
  Serial.print("[OTA-FB] HTTP Status: ");
  Serial.println(code);

  if (code != HTTP_CODE_OK) {
    notifyFallbackEvent(buildId, "failed", "push_request_failed");
    http.end();
    return false;
  }

  int size = http.getSize();
  Serial.print("[OTA-FB] Firmware Size: ");
  Serial.println(size);

  if (size <= 0 || size > ESP.getFreeSketchSpace()) {
    notifyFallbackEvent(buildId, "failed", "invalid_firmware_size");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();

  if (!Update.begin(size)) {
    notifyFallbackEvent(buildId, "failed", "update_begin_failed");
    http.end();
    return false;
  }

  Serial.println("[OTA-FB] Writing firmware...");
  size_t written = Update.writeStream(*stream);

  if (written != size || !Update.end()) {
    notifyFallbackEvent(buildId, "failed", "flash_write_failed");
    http.end();
    return false;
  }

  notifyFallbackEvent(buildId, "success", "push_ota_success");
  http.end();
  return true;
}

/* =====================================================
   OTA DOWNLOAD (PULL MODE)
   ===================================================== */

void performOTA(String buildId) {
  Serial.println("\n========== OTA DOWNLOAD ==========");

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/download/" + buildId;

  Serial.print("[OTA] GET ");
  Serial.println(url);

  http.begin(url);
  int code = http.GET();

  Serial.print("[OTA] HTTP Status: ");
  Serial.println(code);

  /* ---------- FAILURE → FALLBACK ---------- */
  if (code != HTTP_CODE_OK) {
    Serial.println("[OTA] Pull failed → fallback PUSH OTA");
    http.end();

    if (performFallbackOTAPush(buildId)) {
      otaCompleted = true;
      Serial.println("[OTA] Rebooting...");
      delay(2000);
      ESP.restart();
    }

    otaInProgress = false;
    return;
  }

  int size = http.getSize();
  Serial.print("[OTA] Firmware Size: ");
  Serial.println(size);

  if (size <= 0 || size > ESP.getFreeSketchSpace()) {
    Serial.println("[OTA] Invalid firmware size");
    http.end();
    otaInProgress = false;
    return;
  }

  WiFiClient* stream = http.getStreamPtr();

  if (!Update.begin(size)) {
    Serial.println("[OTA] Update.begin FAILED");
    http.end();
    otaInProgress = false;
    return;
  }

  Serial.println("[OTA] Writing firmware...");
  size_t written = Update.writeStream(*stream);

  if (written != size || !Update.end()) {
    Serial.println("[OTA] Pull write failed → fallback");
    http.end();

    if (performFallbackOTAPush(buildId)) {
      otaCompleted = true;
      delay(2000);
      ESP.restart();
    }

    otaInProgress = false;
    return;
  }

  Serial.println("[OTA] OTA SUCCESS");
  otaCompleted = true;
  delay(2000);
  ESP.restart();
}

/* =====================================================
   OTA CHECK
   ===================================================== */

void checkOTA() {
  if (otaInProgress || otaCompleted) return;
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("\n========== OTA CHECK ==========");

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/check?device_id=" + DEVICE_ID;

  http.begin(url);
  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    http.end();
    return;
  }

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, http.getString())) {
    Serial.println("[OTA] JSON parse error");
    http.end();
    return;
  }

  if (!doc["update"]) {
    Serial.println("[OTA] No update available");
    http.end();
    return;
  }

  otaInProgress = true;
  String buildId = doc["build_id"].as<String>();

  Serial.print("[OTA] Update found → build_id = ");
  Serial.println(buildId);

  http.end();
  performOTA(buildId);
}

/* =====================================================
   TELEMETRY
   ===================================================== */

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;

  StaticJsonDocument<512> doc;
  doc["device_id"] = DEVICE_ID;
  JsonObject pins = doc.createNestedObject("pins");

  for (int pin = 0; pin <= 39; pin++) {
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

/* =====================================================
   LED
   ===================================================== */

void blinkLedEvery5Seconds() {
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
  connectWiFi();
}

void loop() {
  unsigned long now = millis();

  if (!otaCompleted && now - lastOtaCheck >= OTA_INTERVAL) {
    lastOtaCheck = now;
    checkOTA();
  }

  if (now - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = now;
    sendTelemetry();
  }

  blinkLedEvery5Seconds();
  delay(10);
}
