#include "Bridge.h"
#include "Config.h"
#include "Display.h"
#include "Joystick.h"
#include <Arduino.h>

// RGB LED pins for ESP32-S3 DevKitC-1
#define RGB_LED_R 18
#define RGB_LED_G 17
#define RGB_LED_B 16

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 USB to BLE Keyboard Bridge           ║");
  Serial.println("║  Supports keyboard + multi-device              ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();

  // Initialize SPI explicitly for display
  Serial.println("Initializing display...");
  delay(500);

  try
  {
    displayInit();
    Serial.println("Display initialized successfully!");
    displayJPEG("/logo.jpg", 0, 0);
    delay(1000);
    displayClearScreen();

    // Start key monitor on core 0
    displayStartKeyMonitor();
  }
  catch (...)
  {
    Serial.println("ERROR: Display initialization failed!");
  }

  delay(1000);
  
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

void loop()
{
  Bridge::loop();
  // displayJoystickValues();
  // joystickControlMouse();
  // displayConnectionStatus();
}