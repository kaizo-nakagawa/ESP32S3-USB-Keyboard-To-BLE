/**
 * @file CommunicationModeExample.h
 * @brief Example showing how to use the modular communication mode system
 * 
 * This file demonstrates how to:
 * 1. Use BLE mode
 * 2. Use ESP-NOW mode
 * 3. Switch between modes at runtime
 */

#ifndef COMMUNICATION_MODE_EXAMPLE_H
#define COMMUNICATION_MODE_EXAMPLE_H

#include "Bridge.h"
#include "BLEMode.h"
#include "ESPNowMode.h"

/**
 * Example: Set up BLE mode
 */
void setupBLEMode() {
  // Create a BLE mode instance
  BLEMode* bleMode = new BLEMode();
  
  // Set it as the active communication mode
  Bridge::setCommunicationMode(bleMode);
  
  // Initialize Bridge (this will initialize BLE)
  Bridge::begin();
}

/**
 * Example: Set up ESP-NOW mode
 */
void setupESPNowMode() {
  // Define target device MAC address
  // Format: {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}
  uint8_t targetMacAddress[] = {0x3C, 0x61, 0x05, 0xFC, 0x1A, 0x7B};
  
  // Create an ESP-NOW mode instance with the target MAC address
  ESPNowMode* espNowMode = new ESPNowMode(targetMacAddress);
  
  // Set it as the active communication mode
  Bridge::setCommunicationMode(espNowMode);
  
  // Initialize Bridge (this will initialize ESP-NOW)
  Bridge::begin();
}

/**
 * Example: Switch modes at runtime
 */
void switchMode(bool useBLE) {
  if (useBLE) {
    setupBLEMode();
  } else {
    setupESPNowMode();
  }
}

/**
 * Example: Use in main.cpp
 * 
 * void setup() {
 *   Serial.begin(115200);
 *   
 *   // Choose which mode to use
 *   setupBLEMode();  // or setupESPNowMode()
 * }
 * 
 * void loop() {
 *   Bridge::loop();
 *   
 *   // The Bridge will handle keyboard/mouse reports
 *   // regardless of which communication mode is active
 * }
 */

#endif // COMMUNICATION_MODE_EXAMPLE_H
