#include "Bridge.h"
#include "Display.h"
#include <hid_usage_keyboard.h>

// Battery voltage divider
#define BAT_ADC 1
#define R1 237000.0
#define R2 121000.0
#define ADC_SAMPLES 32 // Number of samples to average

// HID to ASCII conversion table
static const char HID_TO_ASCII[256] = {
    0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', '0', '\n', 0, 0x08, 0x09, ' ', '-', '=', '[',
    ']', '\\', 0, ';', '\'', '`', ',', '.', '/', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Static member initialization
BleDevice Bridge::bleDevice("Keychron Q1 Wireless", "Espressif");

// Track previous keyboard state to detect releases
static uint8_t prevKeyState[6] = {0, 0, 0, 0, 0, 0};
static uint8_t prevModifier = 0;

void Bridge::begin()
{
  Serial.println("[System] Initializing USB-to-BLE Bridge...");

  // Initialize BLE
  Serial.println("[System] Starting BLE device...");
  bleDevice.begin();

  // Init USB
  Serial.println("[System] Starting USB host...");
  USBManager::setKeyboardCallback(onKeyboardReport);
  USBManager::setMouseCallback(onMouseReport);
  USBManager::setGenericCallback(onGenericReport);
  USBManager::begin();

  // Configure ADC for battery monitoring
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  delay(100);
  Serial.println("[System] Bridge initialized - waiting for connections...");
}

void displayConnectionStatus()
{
  // Display connection status at bottom of screen
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 300);

  if (Bridge::isConnected())
  {
    std::string clientName = Bridge::getConnectedClientName();
    tft.printf("Conn.: %s        ", clientName.c_str());
  }
  else
  {
    tft.printf("Disconnected - Waiting...         ");
  }
}

float readBatteryVoltage()
{
  int sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++)
  {
    sum += analogRead(BAT_ADC);
  }
  int raw = sum / ADC_SAMPLES;

  float v_adc = raw * 3.3 / 4095.0;
  float v_bat = v_adc * (R1 + R2) / R2;
  return v_bat;
}

int batteryLevelToPercentage(float voltage)
{
  // Simple linear mapping for Li-ion battery (3.0V = 0%, 4.2V = 100%)
  if (voltage >= 4.2)
    return 100;
  if (voltage <= 3.0)
    return 0;
  return (int)((voltage - 3.0) / (4.2 - 3.0) * 100);
}

void Bridge::loop()
{
  // Status reporting
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000)
  {
    lastStatusTime = millis();
    Serial.printf("[System] BLE Status: %s\n", bleDevice.isConnected() ? "CONNECTED" : "DISCONNECTED");
    //displayConnectionStatus();
    float batteryVoltage = readBatteryVoltage();
    int batteryPercent = batteryLevelToPercentage(batteryVoltage);
    Serial.printf("[System] Battery Voltage: %.3f V (%d%%)\n", batteryVoltage, batteryPercent);
    bleDevice.reportBatteryLevel(batteryPercent);
  }
}

void Bridge::onKeyboardReport(const uint8_t *data, size_t length)
{
  if (length < sizeof(hid_keyboard_input_report_boot_t))
    return;

  hid_keyboard_input_report_boot_t *kb_report =
      (hid_keyboard_input_report_boot_t *)data;

  // Print intercepted keyboard data
  Serial.printf("[KEYBOARD] Modifier: 0x%02X | Keys: [%02X %02X %02X %02X %02X %02X]\n",
                kb_report->modifier.val, kb_report->key[0], kb_report->key[1],
                kb_report->key[2], kb_report->key[3], kb_report->key[4],
                kb_report->key[5]);

  // Detect key presses (keys in current state but not in previous)
  for (int i = 0; i < 6; i++) {
    if (kb_report->key[i] != 0 && prevKeyState[i] == 0 && kb_report->key[i] != prevKeyState[i]) {
      char asciiKey = HID_TO_ASCII[kb_report->key[i]];
      
      // Handle shift modifier for uppercase/symbols
      if (kb_report->modifier.val & 0x02 || kb_report->modifier.val & 0x20) { // Left or Right Shift
        if (asciiKey >= 'a' && asciiKey <= 'z') {
          asciiKey = asciiKey - 'a' + 'A';
        } else {
          // Handle shifted symbols
          switch (kb_report->key[i]) {
          case 0x1E: asciiKey = '!'; break;
          case 0x1F: asciiKey = '@'; break;
          case 0x20: asciiKey = '#'; break;
          case 0x21: asciiKey = '$'; break;
          case 0x22: asciiKey = '%'; break;
          case 0x23: asciiKey = '^'; break;
          case 0x24: asciiKey = '&'; break;
          case 0x25: asciiKey = '*'; break;
          case 0x26: asciiKey = '('; break;
          case 0x27: asciiKey = ')'; break;
          case 0x2D: asciiKey = '_'; break;
          case 0x2E: asciiKey = '+'; break;
          case 0x2F: asciiKey = '{'; break;
          case 0x30: asciiKey = '}'; break;
          case 0x31: asciiKey = '|'; break;
          case 0x33: asciiKey = ':'; break;
          case 0x34: asciiKey = '"'; break;
          case 0x35: asciiKey = '~'; break;
          case 0x36: asciiKey = '<'; break;
          case 0x37: asciiKey = '>'; break;
          case 0x38: asciiKey = '?'; break;
          }
        }
      }
      
      if (asciiKey != 0) {
        displayKeyPressed(asciiKey);
      }
    }
  }
  
  // Detect key releases (keys in previous state but not in current)
  for (int i = 0; i < 6; i++) {
    if (prevKeyState[i] != 0 && kb_report->key[i] == 0) {
      displayKeyReleased();
    }
  }
  
  // Update previous state
  for (int i = 0; i < 6; i++) {
    prevKeyState[i] = kb_report->key[i];
  }
  prevModifier = kb_report->modifier.val;

  // Forward to BLE
  if (Bridge::bleDevice.isConnected())
  {
    Bridge::bleDevice.sendKeyboard(kb_report->key, kb_report->modifier.val);
  }
}

void Bridge::onMouseReport(const uint8_t *data, size_t length)
{
  if (length < 3)
  {
    return;
  }

  uint8_t buttons = data[0] & 0x07; // Mask to only valid button bits (0-2)
  int8_t x = (int8_t)data[1];       // X movement (signed)
  int8_t y = (int8_t)data[2];       // Y movement (signed)
  int8_t wheel = 0;

  if (length >= 4)
  {
    wheel = (int8_t)data[3];
  }

  // Print intercepted mouse data
  Serial.printf("[MOUSE] Buttons: 0x%02X | X: %d | Y: %d | Wheel: %d\n",
                buttons, x, y, wheel);

  // Forward to BLE
  if (Bridge::bleDevice.isConnected())
  {
    Bridge::bleDevice.sendMouse(buttons, x, y, wheel);
  }
}

void Bridge::onGenericReport(const uint8_t *data, size_t length)
{
  // Handle generic HID reports (consumer control, knobs, media keys, etc)
  if (length < 2)
  {
    return;
  }

  uint8_t reportId = data[0];
  uint8_t consumerCode = data[1];

  // Print raw data
  Serial.printf("[GENERIC] Length: %d, ReportID: 0x%02X, Data: ", length, reportId);
  for (size_t i = 1; i < length && i < 16; i++)
  {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();

  // Parse Consumer Control codes (Report ID 0x04)
  if (reportId == 0x04)
  {
    const char *consumerName = "";

    switch (consumerCode)
    {
    case 0xE2:
      consumerName = "MUTE";
      break;
    case 0xE9:
      consumerName = "VOLUME_UP";
      break;
    case 0xEA:
      consumerName = "VOLUME_DOWN";
      break;
    case 0x00:
      consumerName = "RELEASE";
      break;
    default:
      consumerName = "UNKNOWN";
      break;
    }

    Serial.printf("[CONSUMER] Code: 0x%02X (%s)\n", consumerCode, consumerName);

    // Forward consumer control to BLE when connected
    if (consumerCode != 0x00 && Bridge::bleDevice.isConnected())
    {
      Bridge::bleDevice.sendMedia(consumerCode);
    }
  }
}

void Bridge::sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
  if (bleDevice.isConnected())
  {
    bleDevice.sendMouse(buttons, x, y, wheel);
  }
}

void Bridge::sendJoystickReport(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z)
{
  if (bleDevice.isConnected())
  {
    bleDevice.sendJoystick(buttons, x, y, z);
  }
}

bool Bridge::isConnected()
{
  return bleDevice.isConnected();
}

std::string Bridge::getConnectedClientName()
{
  return bleDevice.getConnectedClientName();
}
