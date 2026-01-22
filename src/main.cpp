#include "Bridge.h"
#include "Config.h"
#include <Arduino.h>

// RGB LED pins for ESP32-S3 DevKitC-1
#define RGB_LED_R 18
#define RGB_LED_G 17
#define RGB_LED_B 16

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 USB to BLE Keyboard Bridge           ║");
  Serial.println("║  Supports keyboard + multi-device              ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();

  Bridge::begin();

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  READY - Connect USB devices via hub           ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

void loop() {
  Bridge::loop();
  delay(10);
}
