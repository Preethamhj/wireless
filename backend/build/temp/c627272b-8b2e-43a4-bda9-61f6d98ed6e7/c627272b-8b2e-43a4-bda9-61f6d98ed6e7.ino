#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// ===== USER CODE START =====
void user_setup() { pinMode(2, OUTPUT); }
void user_loop() { digitalWrite(2, HIGH); }
// ===== USER CODE END =====

void setup() {
  Serial.begin(115200);
  user_setup();
}

void loop() {
  user_loop();
}
