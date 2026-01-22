/**
 * @file BleDevice.h
 * @brief BLE HID Device implementation using NimBLE for keyboard, mouse, and media keys.
 */

#pragma once

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>

/**
 * @class BleDevice
 * @brief Manages Bluetooth Low Energy HID device for keyboard, mouse, and media controls.
 *
 * This class encapsulates all BLE HID functionality using NimBLE-Arduino.
 * It handles initialization, connection management, and HID report transmission.
 */
class BleDevice : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
private:
    NimBLEHIDDevice* hid;
    NimBLECharacteristic* inputMouse;
    NimBLECharacteristic* inputKeyboard;
    NimBLECharacteristic* outputKeyboard;
    NimBLECharacteristic* inputMediaKeys;
    NimBLEAdvertising* advertising;
    std::string deviceName;
    std::string deviceManufacturer;
    bool connected = false;
    uint8_t ledStatus = 0;

    // Mouse movement accumulation for throttling
    int16_t _accumulatedX = 0;
    int16_t _accumulatedY = 0;
    int16_t _accumulatedWheel = 0;
    unsigned long _lastMouseSendTime = 0;
    static const uint16_t MOUSE_SEND_INTERVAL_MS = 5;

public:
    /**
     * @brief Constructor for BleDevice.
     * @param deviceName Bluetooth advertised name (default: "Keychron Q1 Wireless")
     * @param deviceManufacturer Manufacturer name (default: "Espressif")
     */
    BleDevice(std::string deviceName = "Keychron Q1 Wireless", std::string deviceManufacturer = "Espressif");

    /**
     * @brief Initialize BLE HID device and start advertising.
     */
    void begin(void);

    /**
     * @brief Check if a BLE host is currently connected.
     * @return true if connected, false otherwise
     */
    bool isConnected(void);

    /**
     * @brief Send a keyboard HID report.
     * @param keys Array of 6 key codes
     * @param modifiers Modifier byte (shift, ctrl, alt, etc)
     */
    void sendKeyboard(const uint8_t *keys, uint8_t modifiers);

    /**
     * @brief Send a mouse HID report with movement and button data.
     * @param buttons Mouse button states (bit 0=left, bit 1=right, bit 2=middle)
     * @param x Relative X movement (-127 to 127)
     * @param y Relative Y movement (-127 to 127)
     * @param wheel Wheel movement (optional)
     */
    void sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel = 0);

    /**
     * @brief Send a media control (consumer) HID report.
     * @param consumerCode USB HID consumer code (0xE2=Mute, 0xE9=Vol+, 0xEA=Vol-, etc)
     */
    void sendMedia(uint8_t consumerCode);

    /**
     * @brief Get LED status from host (Num Lock, Caps Lock, Scroll Lock).
     * @return LED status byte
     */
    uint8_t getLedStatus() { return ledStatus; }

    /**
     * @brief Check if Num Lock is active.
     * @return true if Num Lock is on
     */
    bool isNumLockOn() { return ledStatus & 0x01; }

    /**
     * @brief Check if Caps Lock is active.
     * @return true if Caps Lock is on
     */
    bool isCapsLockOn() { return ledStatus & 0x02; }

    /**
     * @brief Check if Scroll Lock is active.
     * @return true if Scroll Lock is on
     */
    bool isScrollLockOn() { return ledStatus & 0x04; }

protected:
    virtual void onConnect(NimBLEServer* pServer) override;
    virtual void onDisconnect(NimBLEServer* pServer) override;
    void onWrite(NimBLECharacteristic* pCharacteristic) override;

private:
    /**
     * @brief Send raw keyboard report data.
     */
    void sendKeyboardReport(uint8_t* data, uint8_t len);

    /**
     * @brief Send raw mouse report data.
     */
    void sendMouseReport(uint8_t* data, uint8_t len);

    /**
     * @brief Send raw media report data.
     */
    void sendMediaReport(uint8_t* data, uint8_t len);
}; 