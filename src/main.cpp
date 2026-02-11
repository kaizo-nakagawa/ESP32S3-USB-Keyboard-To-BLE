#include "Bridge.h"
#include "Display.h"
#include "DisplayMutex.h"
// #include "GifPlayer.h"
#include "Joystick.h"
#include "esp_heap_caps.h"
#include <Arduino.h>

void printPsramInfo() {
  Serial.printf("PSRAM total: %u bytes\n", ESP.getPsramSize());
  Serial.printf("PSRAM free : %u bytes\n", ESP.getFreePsram());
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 USB to BLE Keyboard Bridge           ║");
  Serial.println("║  Supports keyboard + multi-device              ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();

  try {
    // Initialize SPI explicitly for display
    Serial.println("Initializing display...");
    delay(3500);
    if (!psramInit()) {
      Serial.println("PSRAM init FAILED!");
    } else {
      Serial.println("PSRAM init OK");
    }

    printPsramInfo();
    // // Initialize display mutex for thread-safe access
    initDisplayMutex();
    displayInit();
    Serial.println("Display initialized successfully!");
    displayJPEG("/logo/logo_key.jpg", 0, 0);
    delay(2000);
    displayClearScreen();
    displayUpdateStatus(false, 0);

    // Start GIF playback on core 2
    // gifPlayerInit("/evernight2.gif");

    // Start key monitor on core 0
    displayStartKeyMonitor();
  } catch (...) {
    Serial.println("ERROR: Display initialization failed!");
  }

  // delay(1000);

  // Initialize joystick
  // Serial.println("Initializing joystick...");
  // joystickInit();

  Bridge::begin();

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  READY - Connect USB devices via hub           ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

void loop() {
  Bridge::loop();
  // displayJoystickValues();
  // joystickControlMouse();
  // displayConnectionStatus();
}