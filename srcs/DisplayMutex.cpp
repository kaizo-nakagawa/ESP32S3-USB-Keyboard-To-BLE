#include "DisplayMutex.h"
#include <Arduino.h>

SemaphoreHandle_t displayMutex = nullptr;

void initDisplayMutex() {
  if (displayMutex == nullptr) {
    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == nullptr) {
      Serial.println("ERROR: Failed to create display mutex!");
    }
  }
}

bool lockDisplay(uint32_t timeoutMs) {
  if (displayMutex == nullptr) {
    return false;
  }
  
  TickType_t ticks = (timeoutMs == 0) ? 0 : pdMS_TO_TICKS(timeoutMs);
  return xSemaphoreTake(displayMutex, ticks) == pdTRUE;
}

void unlockDisplay() {
  if (displayMutex != nullptr) {
    xSemaphoreGive(displayMutex);
  }
}
