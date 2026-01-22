#include "Bridge.h"
#include <hid_usage_keyboard.h>

// Static member initialization
BleDevice Bridge::bleDevice("Keychron Q1 Wireless", "Espressif");

void Bridge::begin() {
  Serial.println("[System] Initializing USB-to-BLE Bridge...");

  // Initialize BLE
  Serial.println("[System] Starting BLE device...");
  bleDevice.begin();

  // Init USB
  Serial.println("[System] Starting USB host...");
  USBManager::setKeyboardCallback(onKeyboardReport);
  USBManager::setMouseCallback(onMouseReport);
  USBManager::setGenericCallback(onGenericReport);
  USBManager::begin();

  Serial.println("[System] Bridge initialized - waiting for connections...");
}

void Bridge::loop() {
  // Status reporting
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000) {
    lastStatusTime = millis();
    Serial.printf("[System] BLE Status: %s\n", bleDevice.isConnected() ? "CONNECTED" : "DISCONNECTED");
  }
}

void Bridge::onKeyboardReport(const uint8_t *data, size_t length) {
  if (length < sizeof(hid_keyboard_input_report_boot_t))
    return;

  hid_keyboard_input_report_boot_t *kb_report =
      (hid_keyboard_input_report_boot_t *)data;

  // Print intercepted keyboard data
  Serial.printf("[KEYBOARD] Modifier: 0x%02X | Keys: [%02X %02X %02X %02X %02X %02X]\n",
                kb_report->modifier.val, kb_report->key[0], kb_report->key[1],
                kb_report->key[2], kb_report->key[3], kb_report->key[4],
                kb_report->key[5]);

  // Forward to BLE
  if (Bridge::bleDevice.isConnected()) {
    Bridge::bleDevice.sendKeyboard(kb_report->key, kb_report->modifier.val);
  }
}

void Bridge::onMouseReport(const uint8_t *data, size_t length) {
  if (length < 3) {
    return;
  }

  uint8_t buttons = data[0] & 0x07; // Mask to only valid button bits (0-2)
  int8_t x = (int8_t)data[1];       // X movement (signed)
  int8_t y = (int8_t)data[2];       // Y movement (signed)
  int8_t wheel = 0;

  if (length >= 4) {
    wheel = (int8_t)data[3];
  }

  // Print intercepted mouse data
  Serial.printf("[MOUSE] Buttons: 0x%02X | X: %d | Y: %d | Wheel: %d\n",
                buttons, x, y, wheel);

  // Forward to BLE
  if (Bridge::bleDevice.isConnected()) {
    Bridge::bleDevice.sendMouse(buttons, x, y, wheel);
  }
}

void Bridge::onGenericReport(const uint8_t *data, size_t length) {
  // Handle generic HID reports (consumer control, knobs, media keys, etc)
  if (length < 2) {
    return;
  }

  uint8_t reportId = data[0];
  uint8_t consumerCode = data[1];

  // Print raw data
  Serial.printf("[GENERIC] Length: %d, ReportID: 0x%02X, Data: ", length, reportId);
  for (size_t i = 1; i < length && i < 16; i++) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();

  // Parse Consumer Control codes (Report ID 0x04)
  if (reportId == 0x04) {
    const char *consumerName = "";

    switch (consumerCode) {
      case 0xE2:
        consumerName = "MUTE";
        break;
      case 0xE9:
        consumerName = "VOLUME_UP";
        break;
      case 0xEA:
        consumerName = "VOLUME_DOWN";
        break;
      case 0x00:
        consumerName = "RELEASE";
        break;
      default:
        consumerName = "UNKNOWN";
        break;
    }

    Serial.printf("[CONSUMER] Code: 0x%02X (%s)\n", consumerCode, consumerName);

    // Forward consumer control to BLE when connected
    if (consumerCode != 0x00 && Bridge::bleDevice.isConnected()) {
      Bridge::bleDevice.sendMedia(consumerCode);
    }
  }
}
