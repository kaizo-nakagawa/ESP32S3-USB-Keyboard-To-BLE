#ifndef ESPNOW_MODE_H
#define ESPNOW_MODE_H

#include "CommunicationMode.h"
#include <cstdint>

/**
 * @class ESPNowMode
 * @brief ESP-NOW implementation of CommunicationMode
 */
class ESPNowMode : public CommunicationMode {
public:
  /// Initialize with target MAC address
  ESPNowMode(const uint8_t targetMacAddress[6]);

  void begin() override;
  void sendKeyboard(const uint8_t *keys, uint8_t modifier) override;
  void sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) override;
  void sendMedia(uint8_t consumerCode) override;
  void sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) override;
  bool isConnected() override;
  std::string getConnectedName() override;
  void reportBatteryLevel(int percentage) override;

private:
  uint8_t targetMacAddress[6];

  // Keyboard report structure for ESP-NOW transmission
  struct EspNowKeyboardReport {
    uint8_t modifier;
    uint8_t keys[6];
  };
};

#endif // ESPNOW_MODE_H
