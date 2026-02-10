#include <Arduino.h>
#include "USBKeyboard.h"
#include "Display.h"
#include "DisplayMutex.h"
#include "GifPlayer.h"
#include "Joystick.h"
#include "USBManager.h"

// Forward declarations for USB callbacks
void onKeyboardReport(const uint8_t *data, size_t length);
void onMouseReport(const uint8_t *data, size_t length);
void onGenericReport(const uint8_t *data, size_t length);

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 USB Keyboard Device                  ║");
  Serial.println("║  Acts as keyboard to host computer             ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();

  try
  {
    // Initialize SPI explicitly for display
    // Serial.println("Initializing display...");
    // delay(500);

    // // Initialize display mutex for thread-safe access
    // initDisplayMutex();
    displayInit();
    // Serial.println("Display initialized successfully!");
    displayJPEG("/logo.jpg", 0, 0);
    delay(1000);
    displayClearScreen();

    // Start GIF playback on core 2
    // gifPlayerInit("/evernight2.gif");

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

  // Initialize USB Keyboard device
  USBKeyboard::begin();

  // Initialize USB Host to read input devices
  USBManager::setKeyboardCallback(onKeyboardReport);
  USBManager::setMouseCallback(onMouseReport);
  USBManager::setGenericCallback(onGenericReport);
  USBManager::begin();

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║  READY - Connect input device to hub            ║");
  Serial.println("║  Connect ESP32 to host computer via USB        ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

void onKeyboardReport(const uint8_t *data, size_t length)
{
  if (length < 8)
    return;

  uint8_t modifier = data[0];
  const uint8_t *keys = &data[2]; // Keys start at byte 2

  if (USBKeyboard::isConnected())
  {
    USBKeyboard::sendReport(keys, modifier);
  }

  Serial.printf("[KEYBOARD] Modifier: 0x%02X | Keys: [%02X %02X %02X %02X %02X %02X]\n",
                modifier, keys[0], keys[1], keys[2], keys[3], keys[4], keys[5]);
}

void onMouseReport(const uint8_t *data, size_t length)
{
  if (length < 3)
    return;

  // Mouse support can be added here if needed
  Serial.println("[MOUSE] Input detected (not yet forwarded)");
}

void onGenericReport(const uint8_t *data, size_t length)
{
  Serial.println("[GENERIC] Input detected (not yet forwarded)");
}

void loop()
{
  // Status reporting
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000)
  {
    lastStatusTime = millis();
    Serial.printf("[System] USB Keyboard Connected: %s\n",
                  USBKeyboard::isConnected() ? "YES" : "NO");
  }
  // displayJoystickValues();
  // joystickControlMouse();
  // displayConnectionStatus();
}
