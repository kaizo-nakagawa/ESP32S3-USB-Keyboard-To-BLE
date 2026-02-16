#include "Joystick.h"
#include "Display.h"
#include "Bridge.h"
#include <Arduino.h>

extern TFT_eSPI tft;

// Variables to store raw ADC values
int lastX = 0;
int lastY = 0;
bool lastButton = true;
bool lastMouseButton = false;

// Debounce tracking - prevents repeated keypresses
bool xMinTriggered = false;
bool xMaxTriggered = false;
bool yMinTriggered = false;
bool yMaxTriggered = false;
unsigned long lastKeyTime = 0;
#define DEBOUNCE_MS 300  // Minimum time between keypresses

// Deadzone threshold to filter ADC noise
#define DEADZONE 100

// Mouse control settings
#define MOUSE_CENTER 2048      // Center point of analog range (0-4095)
#define MOUSE_SENSITIVITY 0.01 // Adjust this to change mouse speed (0.01-0.1)
#define MOUSE_MAX_SPEED 10     // Maximum pixels per update
#define XMIN 1000
#define XMAX 2600
#define YMIN 1200
#define YMAX 2800

// USB HID Key codes
#define KEY_HOME 0x4A
#define KEY_END 0x4D
#define KEY_PAGE_UP 0x4B
#define KEY_PAGE_DOWN 0x4E
#define KEY_PRINT_SCREEN 0x46

// Joystick mode flag
// Set to true for joystick mode, false for mouse mode
bool joystickMode = true;

void joystickInit()
{
  // Configure analog pins
  pinMode(JOYSTICK_VRX_PIN, INPUT);
  pinMode(JOYSTICK_VRY_PIN, INPUT);

  // Configure button pin with pull-up (button is active low)
  pinMode(JOYSTICK_BTN_PIN, INPUT_PULLUP);

  Serial.println("Joystick initialized");
  Serial.printf("  VRx: GPIO%d\n", JOYSTICK_VRX_PIN);
  Serial.printf("  VRy: GPIO%d\n", JOYSTICK_VRY_PIN);
  Serial.printf("  Button: GPIO%d\n", JOYSTICK_BTN_PIN);
}

int joystickReadX()
{
  return analogRead(JOYSTICK_VRX_PIN);
}

int joystickReadY()
{
  return analogRead(JOYSTICK_VRY_PIN);
}

bool joystickReadButton()
{
  // Button is active low with pull-up
  return !digitalRead(JOYSTICK_BTN_PIN);
}

void displayJoystickValues()
{
  int x = joystickReadX();
  int y = joystickReadY();
  bool button = joystickReadButton();
  unsigned long now = millis();

  // Reset trigger flags when joystick returns to center
  if (x > XMIN + DEADZONE && x < XMAX - DEADZONE)
  {
    xMinTriggered = false;
    xMaxTriggered = false;
  }
  if (y > YMIN + DEADZONE && y < YMAX - DEADZONE)
  {
    yMinTriggered = false;
    yMaxTriggered = false;
  }

  // X-axis: END key (left) / HOME key (right)
  if (x <= XMIN && !xMinTriggered && (now - lastKeyTime > DEBOUNCE_MS))
  {
    Serial.println("SENDING END");
    Bridge::sendKey(KEY_END);
    xMinTriggered = true;
    lastKeyTime = now;
  }
  else if (x >= XMAX && !xMaxTriggered && (now - lastKeyTime > DEBOUNCE_MS))
  {
    Serial.println("SENDING HOME");
    Bridge::sendKey(KEY_HOME);
    xMaxTriggered = true;
    lastKeyTime = now;
  }

  // Y-axis: PAGE UP (up) / PAGE DOWN (down)
  if (y <= YMIN && !yMinTriggered && (now - lastKeyTime > DEBOUNCE_MS))
  {
    Serial.println("SENDING PAGE UP");
    Bridge::sendKey(KEY_PAGE_UP);
    yMinTriggered = true;
    lastKeyTime = now;
  }
  else if (y >= YMAX && !yMaxTriggered && (now - lastKeyTime > DEBOUNCE_MS))
  {
    Serial.println("SENDING PAGE DOWN");
    Bridge::sendKey(KEY_PAGE_DOWN);
    yMaxTriggered = true;
    lastKeyTime = now;
  }

  // Button: PRINT SCREEN (on press only, not release)
  if (button && !lastButton && (now - lastKeyTime > DEBOUNCE_MS))
  {
    Serial.println("SENDING PRINT SCREEN");
    Bridge::sendKey(KEY_PRINT_SCREEN);
    lastKeyTime = now;
  }
  
  lastButton = button;
}

void joystickControlMouse()
{
  int x = joystickReadX();
  int y = joystickReadY();
  bool button = joystickReadButton();

  if (joystickMode)
  {
    // JOYSTICK MODE
    // Map 0-4095 to 0-255 for joystick axes (127 is center)
    uint8_t joyX = 255 - ((x >> 4) & 0xFF);    // Convert 0-4095 to 0-255, inverted
    uint8_t joyY = (y >> 4) & 0xFF;            // Convert 0-4095 to 0-255
    uint8_t joyButtons = button ? 0x01 : 0x00; // Button 1

    // Send joystick report
    Bridge::sendJoystickReport(joyButtons, joyX, joyY, 127);
  }
  else
  {
    // MOUSE MODE (original implementation)
    // Convert analog readings to mouse movement
    // Map from 0-4095 to -1024 to +1024 (centered at 2048)
    int xDelta = MOUSE_CENTER - x; // Inverted X axis
    int yDelta = y - MOUSE_CENTER;

    // Apply deadzone to prevent drift
    if (abs(xDelta) < 100)
      xDelta = 0;
    if (abs(yDelta) < 100)
      yDelta = 0;

    // Scale movement with sensitivity and limit to max speed
    int8_t mouseX = constrain((int8_t)(xDelta * MOUSE_SENSITIVITY), -MOUSE_MAX_SPEED, MOUSE_MAX_SPEED);
    int8_t mouseY = constrain((int8_t)(yDelta * MOUSE_SENSITIVITY), -MOUSE_MAX_SPEED, MOUSE_MAX_SPEED);

    // Convert button press to mouse button byte (bit 0 = left click)
    uint8_t mouseButtons = button ? 0x01 : 0x00;

    // Send mouse report if movement or button state changed
    if (mouseX != 0 || mouseY != 0 || button != lastMouseButton)
    {
      Bridge::sendMouseReport(mouseButtons, mouseX, mouseY, 0);
      lastMouseButton = button;
    }
  }
}

void joystickToggleMode()
{
  joystickMode = !joystickMode;
  Serial.printf("Joystick mode switched to: %s\n", joystickMode ? "JOYSTICK" : "MOUSE");

  // Update display
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 90);
  tft.printf("Mode: %s   ", joystickMode ? "JOYSTICK" : "MOUSE  ");
}

bool joystickGetMode()
{
  return joystickMode;
}
