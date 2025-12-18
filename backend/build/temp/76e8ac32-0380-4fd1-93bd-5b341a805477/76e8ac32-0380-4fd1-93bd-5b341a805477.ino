#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// ===== USER CODE START =====
// Write your ESP32 firmware code here
void setup() {}
void loop() {}

// ===== USER CODE END =====

void setup() {
  Serial.begin(115200);
  user_setup();
}

void loop() {
  user_loop();
}
