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

// Track previous keyboard state to detect releases
static uint8_t prevKeyState[6] = {0, 0, 0, 0, 0, 0};
static uint8_t prevModifier = 0;

// Define static member variable
CommunicationMode* Bridge::communicationMode = nullptr;

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

void Bridge::setCommunicationMode(CommunicationMode* mode)
{
  communicationMode = mode;
}

void Bridge::begin()
{
  if (!communicationMode)
    return;

  // Initialize communication mode
  communicationMode->begin();

  // Initialize USB
  USBManager::setKeyboardCallback(onKeyboardReport);
  USBManager::setMouseCallback(onMouseReport);
  USBManager::setGenericCallback(onGenericReport);
  USBManager::begin();

  // Configure ADC for battery monitoring
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  delay(100);
  float batteryVoltage = readBatteryVoltage();
  int batteryPercent = batteryLevelToPercentage(batteryVoltage);
  displayUpdateStatus(communicationMode->isConnected(), batteryPercent, 0xff);
}

void Bridge::loop()
{
  if (!communicationMode)
    return;

  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000)
  {
    lastStatusTime = millis();
    float batteryVoltage = readBatteryVoltage();
    int batteryPercent = batteryLevelToPercentage(batteryVoltage);
    displayUpdateStatus(communicationMode->isConnected(), batteryPercent, 0xff);
    communicationMode->reportBatteryLevel(batteryPercent);
  }
}

hid_keyboard_input_report_boot_t *showKeyboardReport(const uint8_t *data, size_t length)
{
  if (length < sizeof(hid_keyboard_input_report_boot_t))
    return nullptr;

  hid_keyboard_input_report_boot_t *kb_report =
      (hid_keyboard_input_report_boot_t *)data;

  // Detect newly pressed keys by finding key codes in current that weren't in previous
  for (int i = 0; i < 6; i++)
  {
    if (kb_report->key[i] != 0)
    {
      bool keyWasPressed = false;
      // Check if this key code was already in the previous state
      for (int j = 0; j < 6; j++)
      {
        if (prevKeyState[j] == kb_report->key[i])
        {
          keyWasPressed = true;
          break;
        }
      }

      // If key is new, display it
      if (!keyWasPressed)
      {
        char asciiKey = HID_TO_ASCII[kb_report->key[i]];

        // Handle shift modifier for uppercase/symbols
        if (kb_report->modifier.val & 0x02 || kb_report->modifier.val & 0x20)
        { // Left or Right Shift
          if (asciiKey >= 'a' && asciiKey <= 'z')
          {
            asciiKey = asciiKey - 'a' + 'A';
          }
          else
          {
            // Handle shifted symbols
            switch (kb_report->key[i])
            {
            case 0x1E:
              asciiKey = '!';
              break;
            case 0x1F:
              asciiKey = '@';
              break;
            case 0x20:
              asciiKey = '#';
              break;
            case 0x21:
              asciiKey = '$';
              break;
            case 0x22:
              asciiKey = '%';
              break;
            case 0x23:
              asciiKey = '^';
              break;
            case 0x24:
              asciiKey = '&';
              break;
            case 0x25:
              asciiKey = '*';
              break;
            case 0x26:
              asciiKey = '(';
              break;
            case 0x27:
              asciiKey = ')';
              break;
            case 0x2D:
              asciiKey = '_';
              break;
            case 0x2E:
              asciiKey = '+';
              break;
            case 0x2F:
              asciiKey = '{';
              break;
            case 0x30:
              asciiKey = '}';
              break;
            case 0x31:
              asciiKey = '|';
              break;
            case 0x33:
              asciiKey = ':';
              break;
            case 0x34:
              asciiKey = '"';
              break;
            case 0x35:
              asciiKey = '~';
              break;
            case 0x36:
              asciiKey = '<';
              break;
            case 0x37:
              asciiKey = '>';
              break;
            case 0x38:
              asciiKey = '?';
              break;
            }
          }
        }

        if (asciiKey != 0)
        {
          displayKeyPressed(asciiKey);
        }
        else
        {
          displayKeyPressed(' '); // Unknown key placeholder
        }
      }
    }
  }

  // Detect key releases (keys in previous state but not in current)
  for (int i = 0; i < 6; i++)
  {
    if (prevKeyState[i] != 0)
    {
      bool keyIsStillPressed = false;
      // Check if this previous key code is in the current state
      for (int j = 0; j < 6; j++)
      {
        if (kb_report->key[j] == prevKeyState[i])
        {
          keyIsStillPressed = true;
          break;
        }
      }

      if (!keyIsStillPressed)
      {
        displayKeyReleased();
      }
    }
  }

  // Update previous state
  for (int i = 0; i < 6; i++)
  {
    prevKeyState[i] = kb_report->key[i];
  }
  prevModifier = kb_report->modifier.val;
  return kb_report;
}

void Bridge::onKeyboardReport(const uint8_t *data, size_t length)
{
  if (!communicationMode)
    return;

  hid_keyboard_input_report_boot_t *kb_report = showKeyboardReport(data, length);
  if (!kb_report)
    return;

  // Send individual reports for each newly pressed key
  // This ensures receivers that only process keys[0] still detect all keypresses
  for (int i = 0; i < 6; i++)
  {
    if (kb_report->key[i] != 0)
    {
      bool keyWasPressed = false;
      for (int j = 0; j < 6; j++)
      {
        if (prevKeyState[j] == kb_report->key[i])
        {
          keyWasPressed = true;
          break;
        }
      }

      // Send newly pressed key
      if (!keyWasPressed)
      {
        uint8_t singleKeyReport[6] = {kb_report->key[i], 0, 0, 0, 0, 0};
        communicationMode->sendKeyboard(singleKeyReport, kb_report->modifier.val);
        delay(5); // Small delay to ensure receiver processes each key
      }
    }
  }

  // Then send the full current state report
  communicationMode->sendKeyboard(kb_report->key, kb_report->modifier.val);
}

void Bridge::onMouseReport(const uint8_t *data, size_t length)
{
  if (!communicationMode)
    return;

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

  // Forward to communication mode
  communicationMode->sendMouse(buttons, x, y, wheel);
}

void Bridge::onGenericReport(const uint8_t *data, size_t length)
{
  if (!communicationMode)
    return;

  // Handle generic HID reports (consumer control, knobs, media keys, etc)
  if (length < 2)
  {
    return;
  }

  uint8_t reportId = data[0];
  uint8_t consumerCode = data[1];

  // Handle Consumer Control codes (Report ID 0x04)
  if (reportId == 0x04)
  {
    // Forward to communication mode
    communicationMode->sendMedia(consumerCode);
  }
}

void Bridge::sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
  if (communicationMode)
  {
    communicationMode->sendMouse(buttons, x, y, wheel);
  }
}

void Bridge::sendJoystickReport(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z)
{
  if (communicationMode)
  {
    communicationMode->sendJoystick(buttons, x, y, z);
  }
}

void Bridge::sendKey(uint8_t keyCode, uint8_t modifier)
{
  if (!communicationMode)
    return;

  uint8_t combinedModifier = modifier | prevModifier;

  // Send key press
  uint8_t keys[6] = {keyCode, 0, 0, 0, 0, 0};
  communicationMode->sendKeyboard(keys, combinedModifier);
  delay(50);

  // Send key release
  uint8_t release[6] = {0, 0, 0, 0, 0, 0};
  communicationMode->sendKeyboard(release, prevModifier);
}

bool Bridge::isConnected()
{
  if (!communicationMode)
    return false;
  return communicationMode->isConnected();
}

std::string Bridge::getConnectedClientName()
{
  if (!communicationMode)
    return "";
  return communicationMode->getConnectedName();
}
