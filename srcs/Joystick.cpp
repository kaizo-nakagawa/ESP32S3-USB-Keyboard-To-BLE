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

// Deadzone threshold to filter ADC noise
#define DEADZONE 30

// Mouse control settings
#define MOUSE_CENTER 2048          // Center point of analog range (0-4095)
#define MOUSE_SENSITIVITY 0.01     // Adjust this to change mouse speed (0.01-0.1)
#define MOUSE_MAX_SPEED 10         // Maximum pixels per update

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
  
  // Only update display if values changed significantly (beyond deadzone)
  bool xChanged = abs(x - lastX) > DEADZONE;
  bool yChanged = abs(y - lastY) > DEADZONE;
  bool buttonChanged = button != lastButton;
  
  if (xChanged || yChanged || buttonChanged)
  {
    lastX = x;
    lastY = y;
    lastButton = button;
    
    // Set text properties
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    
    // Display values - overwrite without clearing to prevent flicker
    tft.setCursor(10, 10);
    tft.printf("VRx: %4d   ", x);
    
    tft.setCursor(10, 35);
    tft.printf("VRy: %4d   ", y);
    
    tft.setCursor(10, 60);
    if (button)
    {
      tft.printf("Btn: PRS   ");
    }
    else
    {
      tft.printf("Btn: REL   ");
    }
    
    // Also print to serial for debugging
    Serial.printf("Joystick - X: %4d, Y: %4d, Button: %s\n", 
                  x, y, button ? "PRESSED" : "RELEASED");
  }
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
    uint8_t joyX = 255 - ((x >> 4) & 0xFF);  // Convert 0-4095 to 0-255, inverted
    uint8_t joyY = (y >> 4) & 0xFF;  // Convert 0-4095 to 0-255
    uint8_t joyButtons = button ? 0x01 : 0x00;  // Button 1
    
    // Send joystick report
    Bridge::sendJoystickReport(joyButtons, joyX, joyY, 127);
  }
  else
  {
    // MOUSE MODE (original implementation)
    // Convert analog readings to mouse movement
    // Map from 0-4095 to -1024 to +1024 (centered at 2048)
    int xDelta = MOUSE_CENTER - x;  // Inverted X axis
    int yDelta = y - MOUSE_CENTER;
    
    // Apply deadzone to prevent drift
    if (abs(xDelta) < 100) xDelta = 0;
    if (abs(yDelta) < 100) yDelta = 0;
    
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
