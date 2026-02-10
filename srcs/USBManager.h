/**
 * @file USBManager.h
 * @brief Manages USB Host and HID Driver functionality.
 */

#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include <Arduino.h>

// Forward declarations for opaque HID types
typedef void* hid_host_device_handle_t;
typedef unsigned int hid_host_driver_event_t;
typedef unsigned int hid_host_interface_event_t;

/** @brief Callback type for keyboard reports. */
typedef void (*KeyboardReportCallback)(const uint8_t *data, size_t length);

/** @brief Callback type for mouse reports. */
typedef void (*MouseReportCallback)(const uint8_t *data, size_t length);

/** @brief Callback type for generic/consumer control reports. */
typedef void (*GenericReportCallback)(const uint8_t *data, size_t length);

class USBManager {
public:
  /**
   * @brief Initializes the USB Host library and HID driver.
   * Spawns background tasks for USB event handling.
   */
  static void begin();

  /** @brief Sets the callback for incoming keyboard reports. */
  static void setKeyboardCallback(KeyboardReportCallback cb) {
    _keyboardCb = cb;
  }

  /** @brief Sets the callback for incoming mouse reports. */
  static void setMouseCallback(MouseReportCallback cb) {
    _mouseCb = cb;
  }

  /** @brief Sets the callback for incoming generic/consumer reports (knob, media keys, etc). */
  static void setGenericCallback(GenericReportCallback cb) {
    _genericCb = cb;
  }

private:
  static KeyboardReportCallback _keyboardCb;
  static MouseReportCallback _mouseCb;
  static GenericReportCallback _genericCb;

  static void usb_lib_task(void *arg);
  static void hid_host_task(void *pvParameters);

  static void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event, void *arg);
  static void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                                    const hid_host_driver_event_t event,
                                    void *arg);
  static void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_interface_event_t event,
                              void *arg);
};

#endif // USB_MANAGER_H
