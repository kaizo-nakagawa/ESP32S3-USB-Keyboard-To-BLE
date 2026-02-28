# Communication Mode Refactoring

## Overview

The codebase has been refactored to support pluggable communication modes (BLE, ESP-NOW, or future protocols) through a clean modular architecture.

## Architecture

### Core Components

**CommunicationMode.h** - Abstract interface
```cpp
class CommunicationMode {
  virtual void begin() = 0;
  virtual void sendKeyboard(const uint8_t *keys, uint8_t modifier) = 0;
  virtual void sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) = 0;
  virtual void sendMedia(uint8_t consumerCode) = 0;
  virtual void sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) = 0;
  virtual bool isConnected() = 0;
  virtual std::string getConnectedName() = 0;
  virtual void reportBatteryLevel(int percentage) = 0;
};
```

**BLEMode** - Bluetooth Low Energy implementation
- Located in BLEMode.h and BLEMode.cpp
- Wraps the existing BleDevice class
- Supports keyboard, mouse, media, and joystick reports
- Reports battery level to connected device

**ESPNowMode** - ESP-NOW wireless protocol implementation
- Located in ESPNowMode.h and ESPNowMode.cpp
- Uses ESP-NOW for low-latency communication
- Configurable target MAC address
- Supports keyboard, mouse, media, and joystick reports

**Bridge** - Unified interface (refactored)
- Now mode-agnostic
- Delegates to the active CommunicationMode
- Set active mode with `Bridge::setCommunicationMode()`
- All report callbacks work with any mode

## Usage

### Basic Setup (BLE Mode)

```cpp
#include "Bridge.h"
#include "BLEMode.h"

void setup() {
  BLEMode* bleMode = new BLEMode();
  Bridge::setCommunicationMode(bleMode);
  Bridge::begin();
}

void loop() {
  Bridge::loop();
}
```

### Basic Setup (ESP-NOW Mode)

```cpp
#include "Bridge.h"
#include "ESPNowMode.h"

void setup() {
  uint8_t targetMac[] = {0x3C, 0x61, 0x05, 0xFC, 0x1A, 0x7B};
  ESPNowMode* espNowMode = new ESPNowMode(targetMac);
  Bridge::setCommunicationMode(espNowMode);
  Bridge::begin();
}

void loop() {
  Bridge::loop();
}
```

### Runtime Mode Switching

```cpp
void switchToESPNow() {
  uint8_t targetMac[] = {0x3C, 0x61, 0x05, 0xFC, 0x1A, 0x7B};
  ESPNowMode* espNowMode = new ESPNowMode(targetMac);
  Bridge::setCommunicationMode(espNowMode);
  Bridge::begin();
}

void switchToBLE() {
  BLEMode* bleMode = new BLEMode();
  Bridge::setCommunicationMode(bleMode);
  Bridge::begin();
}
```

## Benefits

1. **Clean Separation of Concerns** - Communication logic is isolated from input handling
2. **Easy to Add New Modes** - Just implement the CommunicationMode interface
3. **No Code Duplication** - Keyboard/mouse handling logic is shared
4. **Runtime Flexibility** - Switch modes without recompilation
5. **Testable** - Can mock the CommunicationMode for testing
6. **Reduced Coupling** - Bridge doesn't know about WiFi, BLE, or ESP-NOW details

## Adding a New Communication Mode

To add a new communication mode (e.g., WiFi Direct):

1. Create `WiFiDirectMode.h` header file
2. Inherit from CommunicationMode
3. Implement all virtual methods
4. Create `WiFiDirectMode.cpp` with implementation
5. Use like any other mode: `Bridge::setCommunicationMode(new WiFiDirectMode());`

Example skeleton:

```cpp
// WiFiDirectMode.h
class WiFiDirectMode : public CommunicationMode {
public:
  void begin() override;
  void sendKeyboard(const uint8_t *keys, uint8_t modifier) override;
  void sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) override;
  void sendMedia(uint8_t consumerCode) override;
  void sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) override;
  bool isConnected() override;
  std::string getConnectedName() override;
  void reportBatteryLevel(int percentage) override;
};
```

## Files Modified/Created

### Modified
- **Bridge.h** - Changed architecture to use CommunicationMode pointer
- **Bridge.cpp** - Refactored to delegate to communication mode

### Created
- **CommunicationMode.h** - Abstract interface
- **BLEMode.h** - BLE implementation header
- **BLEMode.cpp** - BLE implementation
- **ESPNowMode.h** - ESP-NOW implementation header
- **ESPNowMode.cpp** - ESP-NOW implementation
- **CommunicationModeExample.h** - Usage examples

## Backward Compatibility

The public API of Bridge remains the same:
- `Bridge::begin()`
- `Bridge::loop()`
- `Bridge::onKeyboardReport()`, `onMouseReport()`, `onGenericReport()`
- `Bridge::sendMouseReport()`, `sendJoystickReport()`, `sendKey()`
- `Bridge::isConnected()`, `getConnectedClientName()`

Only addition is `Bridge::setCommunicationMode()` which must be called before `Bridge::begin()`.
