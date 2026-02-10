/**
 * @file Bridge.h
 * @brief Main application bridge that connects USB HID input to BLE HID output.
 *
 * Bridges USB keyboard/mouse/knob input to a Bluetooth HID device.
 * Handles USB report parsing and forwarding to BLE.
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#include <Arduino.h>
// #include "USBManager.h"
// #include "BleDevice.h"

/**
 * @class Bridge
 * @brief Main application class that manages USB to BLE bridging.
 */
class Bridge {
public:
  /// Initializes both USB host and BLE device
  static void begin();

  /// Main loop for periodic status updates
  static void loop();

  /// Callback for USB keyboard reports
  static void onKeyboardReport(const uint8_t *data, size_t length);

  /// Callback for USB mouse reports
  static void onMouseReport(const uint8_t *data, size_t length);

  /// Callback for generic USB reports (consumer control, media keys, etc)
  static void onGenericReport(const uint8_t *data, size_t length);

  /// Send mouse report via BLE
  static void sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel = 0);

  /// Send joystick report via BLE
  static void sendJoystickReport(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z = 127);

  /// Check if BLE device is connected
  static bool isConnected();

  /// Get connected client name/address
  static std::string getConnectedClientName();

private:
  // static BleDevice bleDevice;
};

#endif // BRIDGE_H
