#ifndef COMMUNICATION_MODE_H
#define COMMUNICATION_MODE_H

#include <stdint.h>
#include <string>

/**
 * @class CommunicationMode
 * @brief Abstract interface for communication modes (BLE, ESP-NOW, etc)
 */
class CommunicationMode {
public:
  virtual ~CommunicationMode() = default;

  /// Initialize the communication mode
  virtual void begin() = 0;

  /// Send keyboard report
  virtual void sendKeyboard(const uint8_t *keys, uint8_t modifier) = 0;

  /// Send mouse report
  virtual void sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) = 0;

  /// Send media/consumer control report
  virtual void sendMedia(uint8_t consumerCode) = 0;

  /// Send joystick report
  virtual void sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) = 0;

  /// Check if connected to a peer device
  virtual bool isConnected() = 0;

  /// Get connected peer name/address
  virtual std::string getConnectedName() = 0;

  /// Report battery level
  virtual void reportBatteryLevel(int percentage) = 0;
};

#endif // COMMUNICATION_MODE_H
