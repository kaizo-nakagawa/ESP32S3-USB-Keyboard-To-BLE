#include "Bridge.h"
#include "Display.h"
#include "DisplayMutex.h"
#include "BLEMode.h"
#include "ESPNowMode.h"
// #include "GifPlayer.h"
#include "Joystick.h"
#include "esp_heap_caps.h"
#include <Arduino.h>

#define COMM_MODE 1 // 0 = BLE, 1 = ESP-NOW
#define ESPNOW_TARGET_MAC {0x3C, 0x61, 0x05, 0xFC, 0x1A, 0x7B} // ESP-NOW target device MAC address

void printPsramInfo()
{
  Serial.printf("PSRAM total: %u bytes\n", ESP.getPsramSize());
  Serial.printf("PSRAM free : %u bytes\n", ESP.getFreePsram());
}

void initializeCommunicationMode()
{
#if COMM_MODE == 0
  // BLE Mode
  Serial.println("Initializing communication mode: BLE...");
  BLEMode* bleMode = new BLEMode();
  Bridge::setCommunicationMode(bleMode);
  Serial.println("Communication mode set to BLE");

#elif COMM_MODE == 1
  // ESP-NOW Mode
  Serial.println("Initializing communication mode: ESP-NOW...");
  uint8_t targetMac[] = ESPNOW_TARGET_MAC;
  ESPNowMode* espNowMode = new ESPNowMode(targetMac);
  Bridge::setCommunicationMode(espNowMode);
  Serial.printf("Communication mode set to ESP-NOW (target: %02X:%02X:%02X:%02X:%02X:%02X)\n",
                targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5]);

#else
  #error "Unknown COMM_MODE. Use 0 for BLE or 1 for ESP-NOW"
#endif
}

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

  try
  {
    
    // Initialize SPI explicitly for display
    Serial.println("Initializing display...");
    delay(3500);
    if (!psramInit())
    {
      Serial.println("PSRAM init FAILED!");
    }
    else
    {
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
    displayUpdateStatus(false, 0, 0xff);

    // Start GIF playback on core 2
    // gifPlayerInit("/evernight2.gif");

    // Start key monitor on core 0
    displayStartKeyMonitor();
  }
  catch (...)
  {
    Serial.println("ERROR: Display initialization failed!");
  }

  // delay(1000);

  // Initialize joystick
  Serial.println("Initializing joystick...");
  joystickInit();

  // Initialize communication mode (BLE or ESP-NOW)
  initializeCommunicationMode();

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
  displayJoystickValues();
  // joystickControlMouse();
  // displayConnectionStatus();
}
