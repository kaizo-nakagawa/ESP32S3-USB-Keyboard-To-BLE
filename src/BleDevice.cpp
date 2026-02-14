#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <NimBLEDescriptor.h>
#include <NimBLEHIDDevice.h>

#include "BleDevice.h"
#include "Display.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG ""
#else
#include "esp_log.h"
static const char *LOG_TAG = "BLEDevice";
#endif

// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02
#define MOUSE_ID 0x03
#define JOYSTICK_ID 0x04

static const uint8_t _hidReportDescriptor[] = {
    USAGE_PAGE(1), 0x01, // USAGE_PAGE (Generic Desktop Ctrls)
    USAGE(1), 0x06,      // USAGE (Keyboard)
    COLLECTION(1), 0x01, // COLLECTION (Application)
    // ------------------------------------------------- Keyboard
    REPORT_ID(1), KEYBOARD_ID, //   REPORT_ID (1)
    USAGE_PAGE(1), 0x07,       //   USAGE_PAGE (Kbrd/Keypad)
    USAGE_MINIMUM(1), 0xE0,    //   USAGE_MINIMUM (0xE0)
    USAGE_MAXIMUM(1), 0xE7,    //   USAGE_MAXIMUM (0xE7)
    LOGICAL_MINIMUM(1), 0x00,  //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0x01,  //   Logical Maximum (1)
    REPORT_SIZE(1), 0x01,      //   REPORT_SIZE (1)
    REPORT_COUNT(1), 0x08,     //   REPORT_COUNT (8)
    HIDINPUT(1), 0x02,         //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    REPORT_COUNT(1), 0x01,     //   REPORT_COUNT (1) ; 1 byte (Reserved)
    REPORT_SIZE(1), 0x08,      //   REPORT_SIZE (8)
    HIDINPUT(1), 0x01,         //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    REPORT_COUNT(1), 0x05,     //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
    REPORT_SIZE(1), 0x01,      //   REPORT_SIZE (1)
    USAGE_PAGE(1), 0x08,       //   USAGE_PAGE (LEDs)
    USAGE_MINIMUM(1), 0x01,    //   USAGE_MINIMUM (0x01) ; Num Lock
    USAGE_MAXIMUM(1), 0x05,    //   USAGE_MAXIMUM (0x05) ; Kana
    HIDOUTPUT(1), 0x02,        //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    REPORT_COUNT(1), 0x01,     //   REPORT_COUNT (1) ; 3 bits (Padding)
    REPORT_SIZE(1), 0x03,      //   REPORT_SIZE (3)
    HIDOUTPUT(1), 0x01,        //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    REPORT_COUNT(1), 0x06,     //   REPORT_COUNT (6) ; 6 bytes (Keys)
    REPORT_SIZE(1), 0x08,      //   REPORT_SIZE(8)
    LOGICAL_MINIMUM(1), 0x00,  //   LOGICAL_MINIMUM(0)
    LOGICAL_MAXIMUM(1), 0x65,  //   LOGICAL_MAXIMUM(0x65) ; 101 keys
    USAGE_PAGE(1), 0x07,       //   USAGE_PAGE (Kbrd/Keypad)
    USAGE_MINIMUM(1), 0x00,    //   USAGE_MINIMUM (0)
    USAGE_MAXIMUM(1), 0x65,    //   USAGE_MAXIMUM (0x65)
    HIDINPUT(1), 0x00,         //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    END_COLLECTION(0),         // END_COLLECTION
    // ------------------------------------------------- Media Keys
    USAGE_PAGE(1), 0x0C,         // USAGE_PAGE (Consumer)
    USAGE(1), 0x01,              // USAGE (Consumer Control)
    COLLECTION(1), 0x01,         // COLLECTION (Application)
    REPORT_ID(1), MEDIA_KEYS_ID, //   REPORT_ID (3)
    USAGE_PAGE(1), 0x0C,         //   USAGE_PAGE (Consumer)
    LOGICAL_MINIMUM(1), 0x00,    //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0x01,    //   LOGICAL_MAXIMUM (1)
    REPORT_SIZE(1), 0x01,        //   REPORT_SIZE (1)
    REPORT_COUNT(1), 0x10,       //   REPORT_COUNT (16)
    USAGE(1), 0xB5,              //   USAGE (Scan Next Track)     ; bit 0: 1
    USAGE(1), 0xB6,              //   USAGE (Scan Previous Track) ; bit 1: 2
    USAGE(1), 0xB7,              //   USAGE (Stop)                ; bit 2: 4
    USAGE(1), 0xCD,              //   USAGE (Play/Pause)          ; bit 3: 8
    USAGE(1), 0xE2,              //   USAGE (Mute)                ; bit 4: 16
    USAGE(1), 0xE9,              //   USAGE (Volume Increment)    ; bit 5: 32
    USAGE(1), 0xEA,              //   USAGE (Volume Decrement)    ; bit 6: 64
    USAGE(2), 0x23, 0x02,        //   Usage (WWW Home)            ; bit 7: 128
    USAGE(2), 0x94, 0x01,        //   Usage (My Computer) ; bit 0: 1
    USAGE(2), 0x92, 0x01,        //   Usage (Calculator)  ; bit 1: 2
    USAGE(2), 0x2A, 0x02,        //   Usage (WWW fav)     ; bit 2: 4
    USAGE(2), 0x21, 0x02,        //   Usage (WWW search)  ; bit 3: 8
    USAGE(2), 0x26, 0x02,        //   Usage (WWW stop)    ; bit 4: 16
    USAGE(2), 0x24, 0x02,        //   Usage (WWW back)    ; bit 5: 32
    USAGE(2), 0x83, 0x01,        //   Usage (Media sel)   ; bit 6: 64
    USAGE(2), 0x8A, 0x01,        //   Usage (Mail)        ; bit 7: 128
    HIDINPUT(1), 0x02,           //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    END_COLLECTION(0),           // END_COLLECTION

    // ------------------------------------------------- Mouse
    USAGE_PAGE(1),
    0x01,                   // 	USAGE_PAGE (Generic Desktop)
    USAGE(1), 0x02,         // 	USAGE (Mouse)
    COLLECTION(1), 0x01,    // 	COLLECTION (Application)
    USAGE(1), 0x01,         //   	USAGE (Pointer)
    COLLECTION(1), 0x00,    //   	COLLECTION (Physical)
    REPORT_ID(1), MOUSE_ID, //  REPORT_ID (1)
    // ------------------------------------------------- Buttons (Left, Right, Middle, Back, Forward)
    USAGE_PAGE(1), 0x09,      //     USAGE_PAGE (Button)
    USAGE_MINIMUM(1), 0x01,   //     USAGE_MINIMUM (Button 1)
    USAGE_MAXIMUM(1), 0x05,   //     USAGE_MAXIMUM (Button 5)
    LOGICAL_MINIMUM(1), 0x00, //     LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0x01, //     LOGICAL_MAXIMUM (1)
    REPORT_SIZE(1), 0x01,     //     REPORT_SIZE (1)
    REPORT_COUNT(1), 0x05,    //     REPORT_COUNT (5)
    HIDINPUT(1), 0x02,        //     INPUT (Data, Variable, Absolute) ;5 button bits
    // ------------------------------------------------- Padding
    REPORT_SIZE(1), 0x03,  //     REPORT_SIZE (3)
    REPORT_COUNT(1), 0x01, //     REPORT_COUNT (1)
    HIDINPUT(1), 0x03,     //     INPUT (Constant, Variable, Absolute) ;3 bit padding
    // ------------------------------------------------- X/Y position, Wheel
    USAGE_PAGE(1), 0x01,      //     USAGE_PAGE (Generic Desktop)
    USAGE(1), 0x30,           //     USAGE (X)
    USAGE(1), 0x31,           //     USAGE (Y)
    USAGE(1), 0x38,           //     USAGE (Wheel)
    LOGICAL_MINIMUM(1), 0x81, //     LOGICAL_MINIMUM (-127)
    LOGICAL_MAXIMUM(1), 0x7f, //     LOGICAL_MAXIMUM (127)
    REPORT_SIZE(1), 0x08,     //     REPORT_SIZE (8)
    REPORT_COUNT(1), 0x03,    //     REPORT_COUNT (3)
    HIDINPUT(1), 0x06,        //     INPUT (Data, Variable, Relative) ;3 bytes (X,Y,Wheel)
    END_COLLECTION(0),        //   	END_COLLECTION
    END_COLLECTION(0),        // 	END_COLLECTION

    // ------------------------------------------------- Joystick
    USAGE_PAGE(1), 0x01,       // USAGE_PAGE (Generic Desktop)
    USAGE(1), 0x04,            // USAGE (Joystick)
    COLLECTION(1), 0x01,       // COLLECTION (Application)
    REPORT_ID(1), JOYSTICK_ID, //   REPORT_ID (4)
    // ------------------------------------------------- Buttons (8 buttons)
    USAGE_PAGE(1), 0x09,      //   USAGE_PAGE (Button)
    USAGE_MINIMUM(1), 0x01,   //   USAGE_MINIMUM (Button 1)
    USAGE_MAXIMUM(1), 0x08,   //   USAGE_MAXIMUM (Button 8)
    LOGICAL_MINIMUM(1), 0x00, //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0x01, //   LOGICAL_MAXIMUM (1)
    REPORT_SIZE(1), 0x01,     //   REPORT_SIZE (1)
    REPORT_COUNT(1), 0x08,    //   REPORT_COUNT (8)
    HIDINPUT(1), 0x02,        //   INPUT (Data, Variable, Absolute)
    // ------------------------------------------------- X/Y/Z axes
    USAGE_PAGE(1), 0x01,      //   USAGE_PAGE (Generic Desktop)
    USAGE(1), 0x30,           //   USAGE (X)
    USAGE(1), 0x31,           //   USAGE (Y)
    USAGE(1), 0x32,           //   USAGE (Z)
    LOGICAL_MINIMUM(1), 0x00, //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1), 0xFF, //   LOGICAL_MAXIMUM (255)
    REPORT_SIZE(1), 0x08,     //   REPORT_SIZE (8)
    REPORT_COUNT(1), 0x03,    //   REPORT_COUNT (3)
    HIDINPUT(1), 0x02,        //   INPUT (Data, Variable, Absolute)
    END_COLLECTION(0)         // END_COLLECTION
};

/**
 * @brief Constructor for BleDevice
 * @param deviceName Bluetooth advertised name
 * @param deviceManufacturer Manufacturer name
 */
BleDevice::BleDevice(std::string deviceName, std::string deviceManufacturer)
    : hid(nullptr), inputMouse(nullptr), inputJoystick(nullptr),
      inputKeyboard(nullptr), outputKeyboard(nullptr), inputMediaKeys(nullptr),
      advertising(nullptr), deviceName(deviceName),
      deviceManufacturer(deviceManufacturer), connectedClientName(""),
      connected(false), ledStatus(0), _accumulatedX(0), _accumulatedY(0),
      _accumulatedWheel(0), _lastMouseSendTime(0) {}

void BleDevice::begin(void)
{

  NimBLEDevice::init(deviceName);
#ifdef ESP_PLATFORM
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
  NimBLEDevice::setPower(9); /** +9db */
#endif

  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityPasskey(123456);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEServer *pServer = NimBLEDevice::createServer();
  NimBLEService *pService = pServer->createService("ABCD");
  NimBLECharacteristic *pSecureCharacteristic = pService->createCharacteristic("1235", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

  pService->start();
  pSecureCharacteristic->setValue("Hello Secure BLE");

  pServer->setCallbacks(this);

  hid = new NimBLEHIDDevice(pServer);

  inputKeyboard = hid->getInputReport(KEYBOARD_ID);
  outputKeyboard = hid->getOutputReport(KEYBOARD_ID);
  inputMediaKeys = hid->getInputReport(MEDIA_KEYS_ID);
  inputMouse = hid->getInputReport(MOUSE_ID);
  inputJoystick = hid->getInputReport(0x04);

  outputKeyboard->setCallbacks(this);

  hid->setManufacturer(deviceManufacturer);

  hid->setPnp(0x02, 0x05ac, 0x820a, 0x0210);
  hid->setHidInfo(0x00, 0x01);

  hid->setReportMap((uint8_t *)_hidReportDescriptor,
                    sizeof(_hidReportDescriptor));

  hid->startServices();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  // Limit advertise name length
  std::string advertiseName = deviceName;
  if (advertiseName.length() > 20)
  {
    advertiseName = advertiseName.substr(0, 20);
  }

  pAdvertising->setName(advertiseName);
  pAdvertising->setAppearance(HID_KEYBOARD);
  pAdvertising->addServiceUUID(hid->getHidService()->getUUID());
  pAdvertising->enableScanResponse(true);
  pAdvertising->addServiceUUID("ABCD");
  pAdvertising->start();

  ESP_LOGI(LOG_TAG, "BLE HID Advertising started");
}

bool BleDevice::isConnected(void)
{
  return this->connected;
}

void BleDevice::sendKeyboardReport(uint8_t *data, uint8_t len)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue(data, len);
    this->inputKeyboard->notify();
  }
}

void BleDevice::sendMouseReport(uint8_t *data, uint8_t len)
{
  if (this->isConnected())
  {
    this->inputMouse->setValue(data, len);
    this->inputMouse->notify();
  }
}

void BleDevice::sendMediaReport(uint8_t *data, uint8_t len)
{
  if (this->isConnected())
  {
    this->inputMediaKeys->setValue(data, len);
    this->inputMediaKeys->notify();
  }
}

void BleDevice::sendJoystickReport(uint8_t *data, uint8_t len)
{
  if (this->isConnected())
  {
    this->inputJoystick->setValue(data, len);
    this->inputJoystick->notify();
  }
}

void BleDevice::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
{
  this->connected = true;

  // Get client address and store as connection info
  NimBLEAddress clientAddr = connInfo.getAddress();
  char addrStr[18];
  sprintf(addrStr, "%s", clientAddr.toString().c_str());

  connectedClientName = std::string(addrStr);
  displayUpdateStatus(true, -1, 0xff); // Update display: connected, battery unknown

  ESP_LOGD(LOG_TAG, "Client connected: handle=%u, addr=%s",
           connInfo.getConnHandle(), connectedClientName.c_str());
}

void BleDevice::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
{
  this->connected = false;
  connectedClientName = "Disconnected";
  NimBLEDevice::startAdvertising();
  displayUpdateStatus(false, -1, 0xff); // Update display: disconnected, battery unknown
  ESP_LOGD(LOG_TAG, "Client disconnected: handle=%u, reason=%d", connInfo.getConnHandle(), reason);
}

void BleDevice::onAuthenticationComplete(ble_gap_conn_desc *desc)
{
  if (!desc->sec_state.encrypted)
  {
    ESP_LOGE(LOG_TAG, "Encrypt failed — disconnecting");
    NimBLEDevice::getServer()->disconnect(desc->conn_handle);
  }
}

void BleDevice::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
{
  if (pCharacteristic == outputKeyboard)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      ledStatus = value[0];
      displayUpdateStatus(true, -1, ledStatus); // Update display: connected, battery unknown
    }
  }
}

void BleDevice::sendKeyboard(const uint8_t *keys, uint8_t modifiers)
{
  if (!isConnected())
  {
    return;
  }

  // Create HID keyboard report (WITHOUT REPORT_ID - NimBLE handles that)
  // Format: [modifier | reserved | key1 | key2 | key3 | key4 | key5 | key6]
  uint8_t reportData[8];
  reportData[0] = modifiers;
  reportData[1] = 0; // Reserved
  reportData[2] = keys[0];
  reportData[3] = keys[1];
  reportData[4] = keys[2];
  reportData[5] = keys[3];
  reportData[6] = keys[4];
  reportData[7] = keys[5];

  sendKeyboardReport(reportData, 8);
}

void BleDevice::sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
  if (!isConnected())
  {
    return;
  }

  // Accumulate movements
  _accumulatedX += x;
  _accumulatedY += y;
  _accumulatedWheel += wheel;

  // Send accumulated movement at throttled intervals
  unsigned long now = millis();
  if (now - _lastMouseSendTime >= MOUSE_SEND_INTERVAL_MS)
  {
    // Clamp accumulated values to int8_t range
    int8_t sendX = constrain(_accumulatedX, -127, 127);
    int8_t sendY = constrain(_accumulatedY, -127, 127);
    int8_t sendWheel = constrain(_accumulatedWheel, -127, 127);

    // Create HID mouse report (WITHOUT REPORT_ID - NimBLE handles that)
    // Format: [buttons | x | y | wheel]
    uint8_t reportData[4];
    reportData[0] = buttons;
    reportData[1] = sendX;
    reportData[2] = sendY;
    reportData[3] = sendWheel;

    sendMouseReport(reportData, 4);

    // Subtract what we sent from accumulator
    _accumulatedX -= sendX;
    _accumulatedY -= sendY;
    _accumulatedWheel -= sendWheel;

    _lastMouseSendTime = now;
  }
}

void BleDevice::sendMedia(uint8_t consumerCode)
{
  if (!isConnected())
  {
    return;
  }

  // Ignore release codes
  if (consumerCode == 0x00)
  {
    return;
  }

  // Map USB consumer codes to 16-bit bitmask for HID descriptor
  uint16_t mediaKeyCode = 0;

  switch (consumerCode)
  {
  case 0xB5:               // Scan Next Track
    mediaKeyCode = 0x0001; // bit 0
    break;
  case 0xB6:               // Scan Previous Track
    mediaKeyCode = 0x0002; // bit 1
    break;
  case 0xB7:               // Stop
    mediaKeyCode = 0x0004; // bit 2
    break;
  case 0xCD:               // Play/Pause
    mediaKeyCode = 0x0008; // bit 3
    break;
  case 0xE2:               // Mute
    mediaKeyCode = 0x0010; // bit 4
    break;
  case 0xE9:               // Volume Increment
    mediaKeyCode = 0x0020; // bit 5
    break;
  case 0xEA:               // Volume Decrement
    mediaKeyCode = 0x0040; // bit 6
    break;
  default:
    return;
  }

  // Create HID media report as 16-bit little-endian value
  uint8_t reportData[2];
  reportData[0] = (uint8_t)(mediaKeyCode & 0xFF);        // Low byte
  reportData[1] = (uint8_t)((mediaKeyCode >> 8) & 0xFF); // High byte

  sendMediaReport(reportData, 2);
}

void BleDevice::sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z)
{
  if (!isConnected())
  {
    return;
  }

  // Create HID joystick report
  // Format: [buttons (1 byte) | x (1 byte) | y (1 byte) | z (1 byte)]
  uint8_t reportData[4];
  reportData[0] = buttons;
  reportData[1] = x; // 0-255, 127 is center
  reportData[2] = y; // 0-255, 127 is center
  reportData[3] = z; // 0-255, 127 is center

  sendJoystickReport(reportData, 4);
}

void BleDevice::reportBatteryLevel(uint8_t level)
{
  // Clamp level to 0-100 range
  if (level > 100)
  {
    level = 100;
  }

  if (hid)
  {
    hid->setBatteryLevel(level);
    ESP_LOGD(LOG_TAG, "Battery level reported: %d%%", level);
  }
}