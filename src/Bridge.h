/**
 * @file Bridge.h
 * @brief Main application bridge that connects USB HID input to BLE/ESP-NOW output.
 *
 * Bridges USB keyboard/mouse/knob input to a Bluetooth HID device or ESP-NOW peer.
 * Handles USB report parsing and forwarding to the selected communication mode.
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#include "USBManager.h"
#include "CommunicationMode.h"
#include <Arduino.h>

/**
 * @class Bridge
 * @brief Main application class that manages USB to BLE/ESP-NOW bridging.
 */
class Bridge {
public:
  /// Set the communication mode (BLE or ESP-NOW)
  static void setCommunicationMode(CommunicationMode* mode);

  /// Initializes both USB host and the selected communication mode
  static void begin();

  /// Main loop for periodic status updates
  static void loop();

  /// Callback for USB keyboard reports
  static void onKeyboardReport(const uint8_t *data, size_t length);

  /// Callback for USB mouse reports
  static void onMouseReport(const uint8_t *data, size_t length);

  /// Callback for generic USB reports (consumer control, media keys, etc)
  static void onGenericReport(const uint8_t *data, size_t length);

  /// Send mouse report via active communication mode
  static void sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel = 0);

  /// Send joystick report via active communication mode
  static void sendJoystickReport(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z = 127);

  /// Send a single key press and release via active communication mode
  static void sendKey(uint8_t keyCode, uint8_t modifier = 0);

  /// Check if connected to peer device
  static bool isConnected();

  /// Get connected client name/address
  static std::string getConnectedClientName();

private:
  static CommunicationMode* communicationMode;
};

#endif // BRIDGE_H
