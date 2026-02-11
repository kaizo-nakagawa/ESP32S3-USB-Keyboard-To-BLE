#include "GifPlayer.h"
#include "Display.h"
#include "DisplayMutex.h"
#include <AnimatedGIF.h>
#include <SPIFFS.h>
#include <FS.h>

// Global GIF decoder and file handle
static AnimatedGIF gif;
static TaskHandle_t gifTaskHandle = nullptr;
static bool gifRunning = false;
static fs::File gifFile;

// ============ File I/O Callbacks ============

void* GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = SPIFFS.open(fname, "r");
  if (gifFile) {
    *pSize = gifFile.size();
    return (void *)&gifFile;
  }
  return nullptr;
}

void GIFCloseFile(void *pHandle) {
  fs::File *f = static_cast<fs::File *>(pHandle);
  if (f != nullptr) {
    f->close();
  }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead = iLen;
  fs::File *f = static_cast<fs::File *>(pFile->fHandle);
  
  if ((pFile->iSize - pFile->iPos) < iLen) {
    iBytesRead = pFile->iSize - pFile->iPos - 1;
  }
  
  if (iBytesRead <= 0) {
    return 0;
  }
  
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  fs::File *f = static_cast<fs::File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}

// ============ GIF Draw Callback ============

void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *usPalette;
  uint16_t usLine[320];  // For 320 pixel width displays
  int x, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > 320) {  // Adjust based on your display width
    iWidth = 320 - pDraw->iX;
  }
  
  if (iWidth < 1 || pDraw->iY >= 240) {  // Adjust based on your display height
    return;
  }

  usPalette = pDraw->pPalette;
  s = pDraw->pPixels;

  // Handle disposal method
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) {
        s[x] = pDraw->ucBackground;
      }
    }
    pDraw->ucHasTransparency = 0;
  }

  // Convert 8-bit palette pixels to 16-bit RGB565
  if (pDraw->ucHasTransparency) {
    uint8_t c, ucTransparent = pDraw->ucTransparent;
    for (x = 0; x < iWidth; x++) {
      c = *s++;
      if (c != ucTransparent) {
        usLine[x] = usPalette[c];
      }
    }
  } else {
    for (x = 0; x < iWidth; x++) {
      usLine[x] = usPalette[s[x]];
    }
  }

  // Lock display mutex before writing
  if (lockDisplay(50)) {  // 50ms timeout
    tft.setAddrWindow(pDraw->iX, pDraw->iY + pDraw->y, iWidth, 1);
    tft.pushColors(usLine, iWidth, false);
    unlockDisplay();
  }
}

// ============ GIF Playback Task ============

void gifPlaybackTask(void *pvParameters) {
  const char* gifPath = (const char*)pvParameters;
  
  // Initialize AnimatedGIF
  gif.begin(BIG_ENDIAN_PIXELS);
  
  // Open GIF with all required callbacks
  if (gif.open(gifPath, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    Serial.printf("GIF opened successfully, canvas: %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    
    // Play frames while running
    while (gifRunning) {
      if (!gif.playFrame(true, nullptr)) {
        // Frame play finished, restart
        gif.reset();
      }
      yield();
    }
    
    gif.close();
  } else {
    Serial.printf("ERROR: Could not open GIF: %s\n", gifPath);
  }
  
  free((void*)gifPath);
  vTaskDelete(nullptr);
}

// ============ Public API ============

void gifPlayerInit(const char* gifPath) {
  if (gifRunning) {
    gifPlayerStop();
  }

  // Check if file exists
  if (!SPIFFS.exists(gifPath)) {
    Serial.printf("ERROR: GIF file not found: %s\n", gifPath);
    return;
  }

  gifRunning = true;
  
  // Allocate memory for path
  char* pathCopy = (char*)malloc(strlen(gifPath) + 1);
  strcpy(pathCopy, gifPath);
  
  // Create task pinned to core 1
  xTaskCreatePinnedToCore(
    gifPlaybackTask,      // Task function
    "GifPlayback",        // Task name
    8192,                 // Stack size
    (void*)pathCopy,      // Task parameter (path)
    1,                    // Priority
    &gifTaskHandle,       // Task handle
    1                     // Core 1
  );
  
  Serial.printf("GIF playback starting on core 1: %s\n", gifPath);
}

void gifPlayerStop() {
  if (gifRunning && gifTaskHandle != nullptr) {
    gifRunning = false;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    Serial.println("GIF playback stopped");
  }
}

bool gifPlayerIsRunning() {
  return gifRunning;
}
