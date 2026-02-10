#ifndef DISPLAY_MUTEX_H
#define DISPLAY_MUTEX_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Global mutex for display access
extern SemaphoreHandle_t displayMutex;

/**
 * Initialize the display mutex
 */
void initDisplayMutex();

/**
 * Lock display for exclusive access (with timeout)
 * @param timeoutMs Timeout in milliseconds (0 = no wait, portMAX_DELAY = wait forever)
 * @return true if lock acquired, false if timeout
 */
bool lockDisplay(uint32_t timeoutMs = 100);

/**
 * Unlock display after access
 */
void unlockDisplay();

#endif
