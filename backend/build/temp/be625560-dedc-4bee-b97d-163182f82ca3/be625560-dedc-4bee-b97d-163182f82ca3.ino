#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

/* =========================
   CONFIG
   ========================= */

const char* WIFI_SSID = "vivo#1933#97";
const char* WIFI_PASSWORD = "29911114";

const char* BACKEND_BASE_URL = "http://192.168.215.116:8000";
const char* DEVICE_ID = "esp32_01";

#define STATUS_LED_PIN 2

/* =========================
   TIMERS
   ========================= */

unsigned long lastOtaCheck = 0;
unsigned long lastTelemetry = 0;

const unsigned long OTA_INTERVAL = 5000;       // 5 seconds
const unsigned long TELEMETRY_INTERVAL = 10000;

/* =========================
   OTA STATE FLAGS
   ========================= */

bool otaInProgress = false;
bool otaCompleted = false;

/* =========================
   WIFI
   ========================= */

void connectWiFi() {
  Serial.println("========== WIFI ==========");
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\n[WIFI] Timeout, restarting...");
      ESP.restart();
    }
  }

  Serial.println("\n[WIFI] Connected successfully");
  Serial.print("[WIFI] IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("==========================");
}

/* =========================
   OTA CHECK
   ========================= */

void checkOTA() {
  if (otaInProgress || otaCompleted) return;
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("\n========== OTA CHECK ==========");

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/check?device_id=" + DEVICE_ID;
  Serial.print("[OTA] GET ");
  Serial.println(url);

  http.begin(url);
  int code = http.GET();
  Serial.print("[OTA] HTTP Status: ");
  Serial.println(code);

  if (code != 200) {
    http.end();
    return;
  }

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, http.getString())) {
    Serial.println("[OTA] JSON parse failed");
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
  Serial.print("[OTA] Update FOUND, build_id = ");
  Serial.println(buildId);

  http.end();
  performOTA(buildId);
}

/* =========================
   OTA DOWNLOAD
   ========================= */

void performOTA(String buildId) {
  Serial.println("\n========== OTA DOWNLOAD ==========");

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/ota/download/" + buildId;
  Serial.print("[OTA] GET ");
  Serial.println(url);

  http.begin(url);
  int code = http.GET();
  Serial.print("[OTA] Download HTTP Status: ");
  Serial.println(code);

  if (code != 200) {
    Serial.println("[OTA] Download failed");
    otaInProgress = false;
    http.end();
    return;
  }

  int size = http.getSize();
  Serial.print("[OTA] Firmware Size: ");
  Serial.print(size);
  Serial.println(" bytes");

  // 🚨 SAFETY CHECK — prevents partition/bootloader bin flashing
  if (size < 100000) {
    Serial.println("[OTA] Firmware too small — aborting");
    otaInProgress = false;
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();

  if (!Update.begin(size)) {
    Serial.println("[OTA] Update begin failed");
    otaInProgress = false;
    http.end();
    return;
  }

  Serial.println("[OTA] Writing firmware...");
  size_t written = Update.writeStream(*stream);
  Serial.print("[OTA] Bytes written: ");
  Serial.println(written);

  if (written != size) {
    Serial.println("[OTA] Write incomplete — aborting");
    Update.abort();
    otaInProgress = false;
    http.end();
    return;
  }

  if (Update.end()) {
    Serial.println("[OTA] Update SUCCESS");
    otaCompleted = true;
    Serial.println("[OTA] Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("[OTA] Update FAILED");
    Serial.println(Update.errorString());
    otaInProgress = false;
  }

  http.end();
}

/* =========================
   TELEMETRY
   ========================= */

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("\n========== TELEMETRY ==========");

  StaticJsonDocument<512> doc;
  doc["device_id"] = DEVICE_ID;
  JsonObject pins = doc.createNestedObject("pins");

  for (int pin = 0; pin <= 39; pin++) {
    pinMode(pin, INPUT);
    int val = digitalRead(pin);
    pins[String(pin)] = val;

    Serial.print("GPIO ");
    Serial.print(pin);
    Serial.print(" = ");
    Serial.println(val);
  }

  String payload;
  serializeJson(doc, payload);

  Serial.println("[TEL] Payload:");
  Serial.println(payload);

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + "/telemetry/" + DEVICE_ID;
  Serial.print("[TEL] POST ");
  Serial.println(url);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  Serial.print("[TEL] HTTP Status: ");
  Serial.println(code);

  http.end();
}

/* =========================
   SETUP & LOOP
   ========================= */

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

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

  delay(10); // watchdog friendly
}
