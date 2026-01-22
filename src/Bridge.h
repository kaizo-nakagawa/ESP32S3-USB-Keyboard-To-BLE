/**
 * @file Bridge.h
 * @brief Main application bridge that connects USB HID input to BLE HID output.
 *
 * Bridges USB keyboard/mouse/knob input to a Bluetooth HID device.
 * Handles USB report parsing and forwarding to BLE.
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#include "Config.h"
#include "USBManager.h"
#include "BleDevice.h"
#include <Arduino.h>

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

private:
  static BleDevice bleDevice;
};

#endif // BRIDGE_H
