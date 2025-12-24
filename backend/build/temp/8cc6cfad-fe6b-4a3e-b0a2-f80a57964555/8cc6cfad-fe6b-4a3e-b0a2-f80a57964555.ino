#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

/* =====================================================
   CONFIGURATION
   ===================================================== */

const char* WIFI_SSID     = "vivo#1933#97";
const char* WIFI_PASSWORD = "29911114";

const char* BACKEND_BASE_URL = "http://192.168.97.116:8000";
const char* DEVICE_ID = "esp32_01";

#define STATUS_LED_PIN 2

/* =====================================================
   SAFE GPIO LIST (READ-ONLY)
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

const unsigned long OTA_INTERVAL       = 10000;   // 10s
const unsigned long TELEMETRY_INTERVAL = 20000;   // 20s
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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\n[WIFI] Timeout");
      return;
    }
  }

  Serial.println("\n[WIFI] Connected");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
}

/* =====================================================
   OTA EVENT LOGGING
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
  Serial.printf("[API] HTTP %d\n", code);
  http.end();
}

/* =====================================================
   SAFE OTA FLASH WRITER (CORE LOGIC)
   ===================================================== */

bool writeFirmware(HTTPClient &http) {
  int contentLength = http.getSize();
  WiFiClient* stream = http.getStreamPtr();

  Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);

  if (contentLength <= 0) {
    Serial.println("[OTA] Invalid Content-Length");
    return false;
  }

  if (!Update.begin(contentLength)) {
    Serial.printf("[OTA] Update.begin failed (%d)\n", Update.getError());
    return false;
  }

  size_t written = 0;
  uint8_t buffer[1024];
  unsigned long lastLog = millis();

  while (http.connected() && written < contentLength) {
    size_t available = stream->available();
    if (available) {
      int readBytes = stream->readBytes(buffer, min(available, sizeof(buffer)));
      written += Update.write(buffer, readBytes);
    }

    if (millis() - lastLog > 1000) {
      Serial.printf("[OTA] Progress: %d / %d\n", written, contentLength);
      lastLog = millis();
    }
    yield();
  }

  if (written != contentLength) {
    Serial.printf("[OTA] Incomplete write %d/%d\n", written, contentLength);
    Update.abort();
    return false;
  }

  if (!Update.end()) {
    Serial.printf("[OTA] Update.end failed (%d)\n", Update.getError());
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("[OTA] Update not finished");
    return false;
  }

  Serial.println("[OTA] Flash write SUCCESS");
  return true;
}

/* =====================================================
   OTA PULL
   ===================================================== */

void performOTA(String buildId) {
  otaInProgress = true;

  String url = String(BACKEND_BASE_URL) + "/ota/download/" + buildId;
  Serial.println("\n========== OTA PULL ==========");
  Serial.println("[API] GET " + url);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  Serial.printf("[API] HTTP %d\n", code);

  if (code != HTTP_CODE_OK) {
    http.end();
    Serial.println("[OTA] Pull failed → fallback");
    notifyFallbackEvent(buildId, "started", "pull_failed");
    performFallbackOTAPush(buildId);
    return;
  }

  if (!writeFirmware(http)) {
    http.end();
    Serial.println("[OTA] Pull write failed → fallback");
    notifyFallbackEvent(buildId, "started", "pull_failed");
    performFallbackOTAPush(buildId);
    return;
  }

  http.end();
  Serial.println("[OTA] SUCCESS → rebooting");
  delay(2000);
  ESP.restart();
}

/* =====================================================
   OTA PUSH (FALLBACK)
   ===================================================== */

bool performFallbackOTAPush(String buildId) {
  String url = String(BACKEND_BASE_URL) + "/ota/push/" + DEVICE_ID;
  Serial.println("\n========== OTA PUSH ==========");
  Serial.println("[API] POST " + url);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST("{}");
  Serial.printf("[API] HTTP %d\n", code);

  if (code != HTTP_CODE_OK) {
    Serial.println("[OTA-PUSH] Failed");
    http.end();
    return false;
  }

  if (!writeFirmware(http)) {
    notifyFallbackEvent(buildId, "failed", "push_failed");
    http.end();
    return false;
  }

  notifyFallbackEvent(buildId, "success", "push_success");
  http.end();
  Serial.println("[OTA-PUSH] SUCCESS → rebooting");
  delay(2000);
  ESP.restart();
  return true;
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
    Serial.println("[OTA] No update");
    http.end();
    return;
  }

  String buildId = doc["build_id"].as<String>();
  Serial.printf("[OTA] Update FOUND → %s\n", buildId.c_str());

  http.end();
  performOTA(buildId);
}

/* =====================================================
   TELEMETRY
   ===================================================== */

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED || otaInProgress) return;

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
  Serial.printf("[API] HTTP %d\n", code);
  http.end();
}

/* =====================================================
   LED
   ===================================================== */

void blinkLed() {
  if (otaInProgress) return;

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
  Serial.println("this is the ota code ");
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
