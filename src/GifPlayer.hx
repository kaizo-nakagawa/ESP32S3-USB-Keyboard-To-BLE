#ifndef GIF_PLAYER_H
#define GIF_PLAYER_H

#include <Arduino.h>

/**
 * Initialize GIF player on core 2
 * @param gifPath Path to the GIF file (e.g., "/animation.gif")
 */
void gifPlayerInit(const char* gifPath);

/**
 * Stop GIF playback and cleanup
 */
void gifPlayerStop();

/**
 * Check if GIF player is running
 */
bool gifPlayerIsRunning();

#endif
