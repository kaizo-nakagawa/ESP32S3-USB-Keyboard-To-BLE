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
void displayJPEG(const char* filename, int x, int y);
void displayClearScreen();
void displayListSPIFFSFiles();
 
#endif // DISPLAY_H
