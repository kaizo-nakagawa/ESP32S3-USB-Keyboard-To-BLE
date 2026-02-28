#include "BLEMode.h"

BLEMode::BLEMode() : bleDevice("Keychron Q1 Wireless", "Espressif") {
}

void BLEMode::begin() {
  bleDevice.begin();
}

void BLEMode::sendKeyboard(const uint8_t *keys, uint8_t modifier) {
  if (isConnected()) {
    bleDevice.sendKeyboard(keys, modifier);
  }
}

void BLEMode::sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) {
  if (isConnected()) {
    bleDevice.sendMouse(buttons, x, y, wheel);
  }
}

void BLEMode::sendMedia(uint8_t consumerCode) {
  if (isConnected()) {
    bleDevice.sendMedia(consumerCode);
  }
}

void BLEMode::sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) {
  if (isConnected()) {
    bleDevice.sendJoystick(buttons, x, y, z);
  }
}

bool BLEMode::isConnected() {
  return bleDevice.isConnected();
}

std::string BLEMode::getConnectedName() {
  return bleDevice.getConnectedClientName();
}

void BLEMode::reportBatteryLevel(int percentage) {
  bleDevice.reportBatteryLevel(percentage);
}
