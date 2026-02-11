#ifndef DISPLAY_H
#define DISPLAY_H

#include "TFT_eSPI.h"

// External TFT instance
extern TFT_eSPI tft;

// Public functions
void displayInit();
void displayStartKeyMonitor();
void displayKeyPressed(char key);
void displayKeyReleased();
void displayClearScreen();
void displayJPEG(const char *filename, int x, int y);
void displayUpdateStatus(bool isConnected, int batteryPercent, uint8_t ledStatus); // New: thread-safe status update

#endif // DISPLAY_H
