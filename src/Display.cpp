#include "SPI.h"
#include "TFT_eSPI.h"
#include "Display.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <TJpg_Decoder.h>
#include <SPIFFS.h>

#define TFT_BL 9 // TFT backlight pin

TFT_eSPI tft = TFT_eSPI();

unsigned long total = 0;
unsigned long tn = 0;

// Key monitor variables
static QueueHandle_t keyQueue = nullptr;
static char lastKey = '\0';
static unsigned long lastKeyTime = 0;
static const unsigned long KEY_DISPLAY_DURATION = 5000; // 5 seconds
static const unsigned long INACTIVITY_TIMEOUT = 30000; // 30 seconds
static unsigned long lastActivityTime = 0;

// Display region for keys (to avoid full screen flicker)
// Make this larger to fully cover text at different sizes
static const int KEY_DISPLAY_X = 40;
static const int KEY_DISPLAY_Y = 40;
static const int KEY_DISPLAY_WIDTH = 240;
static const int KEY_DISPLAY_HEIGHT = 160;

// FreeRTOS task for key display on core 0
void keyDisplayTask(void *parameter) {
  lastActivityTime = millis(); // Initialize activity timer
  while (1) {
    // Check queue for new keys
    char receivedKey;
    if (xQueueReceive(keyQueue, &receivedKey, portMAX_DELAY)) {
      lastKey = receivedKey;
      lastKeyTime = millis();
      lastActivityTime = millis(); // Reset inactivity timer
      
      // Turn on backlight if it was off
      digitalWrite(TFT_BL, HIGH);
      
      // Clear only the key display region instead of entire screen
      tft.fillRect(KEY_DISPLAY_X, KEY_DISPLAY_Y, KEY_DISPLAY_WIDTH, KEY_DISPLAY_HEIGHT, TFT_WHITE);
      
      // Display the key
      tft.setCursor(tft.width() / 2 - 30, tft.height() / 2 - 40);
      tft.setTextColor(TFT_BLACK);
      tft.setTextSize(10);
      tft.print(receivedKey);
      
      Serial.printf("[DISPLAY] Showing key: %c\n", receivedKey);
    }
    
    // Check if display should be turned off due to inactivity (30 seconds)
    if ((millis() - lastActivityTime) > INACTIVITY_TIMEOUT) {
      lastKey = '\0';
      digitalWrite(TFT_BL, LOW); // Turn off backlight
      tft.fillRect(KEY_DISPLAY_X, KEY_DISPLAY_Y, KEY_DISPLAY_WIDTH, KEY_DISPLAY_HEIGHT, TFT_WHITE);
      Serial.println("[DISPLAY] Display turned off due to inactivity");
      lastActivityTime = millis(); // Reset to avoid repeated logging
    }
    // Check if key display should be cleared (5 second timeout)
    else if (lastKey != '\0' && (millis() - lastKeyTime) > KEY_DISPLAY_DURATION) {
      lastKey = '\0';
      // Clear only the key display region
      tft.fillRect(KEY_DISPLAY_X, KEY_DISPLAY_Y, KEY_DISPLAY_WIDTH, KEY_DISPLAY_HEIGHT, TFT_WHITE);
      
      tft.setCursor(tft.width() / 2 - 50, tft.height() / 2);
      tft.setTextColor(TFT_LIGHTGREY);
      tft.setTextSize(2);
      tft.println("Waiting...");
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Forward declarations for SPIFFS functions
void displayInitSPIFFS();

// Initialize display
void displayInit()
{
    Serial.println("TFT_eSPI library initializing...");
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // Turn on backlight
    tft.init();
    tft.setSwapBytes(true);
    
    Serial.println("TFT initialized!");
    delay(500);

    tft.setRotation(2);
    tft.fillScreen(TFT_WHITE);
    tft.setCursor(tft.width() / 2 - 65, tft.height() / 2);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setTextSize(2);
    tft.println("Keychron Q1");
    Serial.println("Display ready for key input");
    
    // Initialize SPIFFS for JPEG storage
    displayInitSPIFFS();
}

void displayTest(void)
{
    // Test function removed - using key display instead
}
void displayStartKeyMonitor() {
  // Create queue for key events
  keyQueue = xQueueCreate(5, sizeof(char));
  
  if (keyQueue == nullptr) {
    Serial.println("[ERROR] Failed to create key queue!");
    return;
  }
  
  // Create task on core 0
  xTaskCreatePinnedToCore(
    keyDisplayTask,       // Function to run
    "KeyDisplayTask",     // Task name
    2048,                 // Stack size
    nullptr,              // Parameter
    1,                    // Priority
    nullptr,              // Task handle
    0                     // Core 0
  );
  
  Serial.println("[DISPLAY] Key monitor started on core 0");
}

void displayKeyPressed(char key) {
  if (keyQueue != nullptr) {
    xQueueSend(keyQueue, &key, portMAX_DELAY);
  }
}

void displayKeyReleased() {
  // Optional: can be used to clear display on key release
}

// ============================================================================
// SPIFFS JPEG Display Functions
// ============================================================================

// Callback function for TJpgDec to output image data to TFT
static bool tftJpegOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return false;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

// Initialize SPIFFS
void displayInitSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] SPIFFS Mount Failed!");
    return;
  }
  Serial.println("[SPIFFS] Mounted successfully");
}

// Display JPEG image from SPIFFS
void displayJPEG(const char* filename, int x, int y) {
  if (!SPIFFS.exists(filename)) {
    Serial.printf("[ERROR] File not found: %s\n", filename);
    tft.fillScreen(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.println("File not found!");
    return;
  }

  Serial.printf("[DISPLAY] Loading JPEG: %s\n", filename);

  // Set TFT_eSPI as the output device for TJpgDec
  TJpgDec.setCallback(tftJpegOutput);
  
  // Decode and display the JPEG from SPIFFS
  TJpgDec.drawJpg(x, y, filename);
  
  Serial.printf("[DISPLAY] JPEG displayed successfully: %s\n", filename);
}

// Clear the display screen
void displayClearScreen() {
  tft.fillScreen(TFT_WHITE);
  Serial.println("[DISPLAY] Screen cleared");
}

// List all files in SPIFFS (for debugging)
void displayListSPIFFSFiles() {
  Serial.println("[SPIFFS] Files in storage:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  while (file) {
    Serial.printf("  %s - %d bytes\n", file.name(), file.size());
    file = root.openNextFile();
  }
  
  // Print SPIFFS usage info
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  Serial.printf("[SPIFFS] Usage: %d / %d bytes (%.1f%%)\n", 
                usedBytes, totalBytes, (float)usedBytes / totalBytes * 100);
}