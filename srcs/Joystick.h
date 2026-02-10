#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>

// Joystick pin definitions
#define JOYSTICK_VRY_PIN 4   // Analog X axis
#define JOYSTICK_VRX_PIN 15   // Analog Y axis
#define JOYSTICK_BTN_PIN 5  // Digital button

// Joystick initialization
void joystickInit();

// Read joystick values
int joystickReadX();
int joystickReadY();
bool joystickReadButton();

// Display joystick values
void displayJoystickValues();

// Control mouse with joystick
void joystickControlMouse();

// Toggle between joystick and mouse mode
void joystickToggleMode();

// Get current mode (true = joystick, false = mouse)
bool joystickGetMode();

#endif // JOYSTICK_H
