#ifndef BLE_MODE_H
#define BLE_MODE_H

#include "CommunicationMode.h"
#include "BleDevice.h"

/**
 * @class BLEMode
 * @brief BLE implementation of CommunicationMode
 */
class BLEMode : public CommunicationMode {
public:
  BLEMode();

  void begin() override;
  void sendKeyboard(const uint8_t *keys, uint8_t modifier) override;
  void sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) override;
  void sendMedia(uint8_t consumerCode) override;
  void sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) override;
  bool isConnected() override;
  std::string getConnectedName() override;
  void reportBatteryLevel(int percentage) override;

private:
  BleDevice bleDevice;
};

#endif // BLE_MODE_H
