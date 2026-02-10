#include "USBKeyboard.h"

// Define guards to prevent TinyUSB from redefining USB Host HID types
#define _DESC_HID_H_
#define _CLASS_HID_H_
#define _HID_DEVICE_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconflicting-declaration"

#include <USB.h>
#include <USBHIDKeyboard.h>

#pragma GCC diagnostic pop

// Static member initialization
static USBHIDKeyboard usbKeyboardDevice;
void* USBKeyboard::usbKeyboardInternal = (void*)&usbKeyboardDevice;
uint8_t USBKeyboard::reportBuffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void USBKeyboard::begin()
{
  Serial.println("[USB] Initializing USB HID Keyboard device...");
  
  USB.begin();
  usbKeyboardDevice.begin();
  
  Serial.println("[USB] Waiting for USB host connection...");
  
  // Give the host time to recognize the device
  delay(1000);
  Serial.println("[USB] Keyboard device ready!");
}

void USBKeyboard::sendReport(const uint8_t *keys, uint8_t modifiers)
{
  if (!isConnected())
    return;

  // Build HID keyboard report
  // Byte 0: Modifier keys
  // Byte 1: Reserved (always 0)
  // Bytes 2-7: Key codes (up to 6 keys)
  reportBuffer[0] = modifiers;
  reportBuffer[1] = 0; // Reserved byte
  
  // Copy key codes
  if (keys != nullptr)
  {
    for (int i = 0; i < 6; i++)
    {
      reportBuffer[i + 2] = keys[i];
    }
  }
  else
  {
    // Clear keys if null pointer
    for (int i = 0; i < 6; i++)
    {
      reportBuffer[i + 2] = 0;
    }
  }

  // Send the report
  usbKeyboardDevice.write(reportBuffer, 8);
}

bool USBKeyboard::isConnected()
{
  // USBHIDKeyboard is always "connected" once initialized
  // The actual connection status is managed internally
  return true;
}
