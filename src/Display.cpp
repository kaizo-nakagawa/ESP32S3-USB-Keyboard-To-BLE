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
static int keysPressed = 0;
static unsigned long lastKeyTime = 0;
static const unsigned long KEY_IDLE_TIMEOUT = 200; // 200ms
static const char* BONGO_IMAGES[] = {
  "/bongo/1.jpg",  // Idle
  "/bongo/2.jpg",  // Multiple keys pressed
  "/bongo/3.jpg",  // Random images
  "/bongo/4.jpg",
  "/bongo/5.jpg",
  "/bongo/6.jpg",
  "/bongo/7.jpg",
  "/bongo/8.jpg"
};
static const int BONGO_COUNT = 8;
static int currentImage = -1;  // Start with -1 (no image displayed yet)
static const unsigned long KEY_DISPLAY_DURATION = 5000; // 5 seconds
static const unsigned long INACTIVITY_TIMEOUT = 30000; // 30 seconds
static unsigned long lastActivityTime = 0;
static char lastKey = '\0';
// Queue for key display
static QueueHandle_t keyQueue = xQueueCreate(10, sizeof(char));

// Display region for keys (to avoid full screen flicker)
// Make this larger to fully cover text at different sizes
static const int KEY_DISPLAY_X = 40;
static const int KEY_DISPLAY_Y = 40;
static const int KEY_DISPLAY_WIDTH = 240;
static const int KEY_DISPLAY_HEIGHT = 160;

// Queue for image requests (thread-safe communication)
static QueueHandle_t imageQueue = nullptr;
struct ImageRequest {
  int imageIndex;
};

// Mutex for keysPressed counter
static portMUX_TYPE keysMutex = portMUX_INITIALIZER_UNLOCKED;

// FreeRTOS task for image display on core 0
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
    // displayInitSPIFFS();
}

void displayTest(void)
{
    // Test function removed - using key display instead
}
void displayStartKeyMonitor() {
  // Create queue for image requests
  imageQueue = xQueueCreate(10, sizeof(ImageRequest));
  
  if (imageQueue == nullptr) {
    Serial.println("[ERROR] Failed to create image queue!");
    return;
  }
  
  // Initialize time tracking
  lastKeyTime = millis();
  currentImage = -1;  // Start with no image to force initial display
  
  // Create task on core 0 with larger stack for JPEG decoding
  xTaskCreatePinnedToCore(
    keyDisplayTask,       // Function to run
    "KeyDisplayTask",     // Task name
    4096,                 // Stack size (larger for JPEG decoding)
    nullptr,              // Parameter
    1,                    // Priority
    nullptr,              // Task handle
    0                     // Core 0
  );
  
  Serial.println("[DISPLAY] Key monitor started on core 0");
}

void displayKeyPressed(char key) {
  // Update key counter with mutex protection
  portENTER_CRITICAL(&keysMutex);
  keysPressed++;
  int keysPressedCopy = keysPressed;
  portEXIT_CRITICAL(&keysMutex);
  
  lastKeyTime = millis();
  
  // Select image based on key state
  ImageRequest request;
  if (keysPressedCopy == 1) {
    // Single key: show random image from 3-8 (indices 2-7)
    request.imageIndex = 2 + (rand() % 6);
  } else {
    // Multiple keys: show image 2 (index 1)
    request.imageIndex = 1;
  }
  
  // Send request to display task via queue
  if (imageQueue != nullptr) {
    xQueueSend(imageQueue, &request, 0);  // Non-blocking send
  }
  
  Serial.printf("[DISPLAY] Key pressed (total: %d) - Requested image %d\n", keysPressedCopy, request.imageIndex + 1);
}

void displayKeyReleased() {
  // Update key counter with mutex protection
  portENTER_CRITICAL(&keysMutex);
  if (keysPressed > 0) {
    keysPressed--;
  }
  int keysPressedCopy = keysPressed;
  portEXIT_CRITICAL(&keysMutex);
  
  lastKeyTime = millis();
  
  Serial.printf("[DISPLAY] Key released (total: %d)\n", keysPressedCopy);
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