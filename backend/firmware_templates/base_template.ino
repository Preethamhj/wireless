#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// ===== USER CODE START =====
{{ USER_CODE }}
// ===== USER CODE END =====

void setup() {
  Serial.begin(115200);
  user_setup();
}

void loop() {
  user_loop();
}
