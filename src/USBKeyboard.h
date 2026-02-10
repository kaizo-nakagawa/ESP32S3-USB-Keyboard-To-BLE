#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <Arduino.h>

/**
 * @class USBKeyboard
 * @brief USB HID Keyboard device implementation for ESP32-S3
 * 
 * Makes ESP32-S3 appear as a USB keyboard to host computer.
 */
class USBKeyboard {
public:
    /**
     * @brief Initialize USB keyboard device
     */
    static void begin();

    /**
     * @brief Send a complete keyboard report
     * @param keys Array of up to 6 key codes
     * @param modifiers Modifier byte
     */
    static void sendReport(const uint8_t *keys, uint8_t modifiers);

    /**
     * @brief Check if USB keyboard is connected to host
     */
    static bool isConnected();

private:
    // Opaque pointer to USB HID keyboard device
    // Real implementation uses USBHIDKeyboard internally
    static void* usbKeyboardInternal;
    static uint8_t reportBuffer[8]; // HID keyboard report buffer
};

#endif // USB_KEYBOARD_H
