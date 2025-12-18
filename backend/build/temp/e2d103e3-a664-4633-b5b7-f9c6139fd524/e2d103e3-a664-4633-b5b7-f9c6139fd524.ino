#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// ===== USER CODE START =====
#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("ESP32 Intermediate Compile Test Started");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}

// ===== USER CODE END =====

void setup() {
  Serial.begin(115200);
  user_setup();
}

void loop() {
  user_loop();
}
