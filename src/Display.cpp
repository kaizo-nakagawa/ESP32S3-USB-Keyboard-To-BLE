#include "Display.h"
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eFEX.h>
#include <TFT_eSPI.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define TFT_BL 9

TFT_eSPI tft;
TFT_eFEX fex(&tft);

// =====================================================
// STATE
// =====================================================

static int keysPressed = 0;
static unsigned long lastKeyTime = 0;
static unsigned long lastActivityTime = 0;

static const unsigned long KEY_IDLE_TIMEOUT = 200;
static const unsigned long INACTIVITY_TIMEOUT = 120000;

// -----------------------------------------------------

static const char *BONGO_IMAGES[] = {
    "/bongo/1.jpg", "/bongo/2.jpg", "/bongo/3.jpg", "/bongo/4.jpg",
    "/bongo/5.jpg", "/bongo/6.jpg", "/bongo/7.jpg", "/bongo/8.jpg"};

static const int BONGO_COUNT = sizeof(BONGO_IMAGES) / sizeof(BONGO_IMAGES[0]);

static int currentImage = -1;

// =====================================================
// PSRAM CACHE
// =====================================================

static uint8_t *bongoCache[BONGO_COUNT] = {0};
static size_t bongoSize[BONGO_COUNT] = {0};

static uint8_t *btIconConnected = nullptr;
static size_t btIconConnectedSize = 0;

static uint8_t *btIconDisconnected = nullptr;
static size_t btIconDisconnectedSize = 0;

static uint8_t *capsIcon = nullptr;
static size_t capsIconSize = 0;

static const int IMAGE_PADDING = 120;

// =====================================================
// SYNC
// =====================================================

SemaphoreHandle_t tftMutex;
SemaphoreHandle_t keysMutex;

// =====================================================
// QUEUE
// =====================================================

static QueueHandle_t imageQueue = nullptr;
static QueueHandle_t statusBarQueue = nullptr;

struct ImageRequest
{
  int imageIndex;
  char keyChar;
};

struct StatusBarRequest
{
  bool isConnected;
  int batteryPercent;
  uint8_t ledStatus;
};

// =====================================================

static const int STATUSBAR_HEIGHT = 40;

static bool lastConnectionState = false;
static int lastBatteryPercent = -1;
static uint8_t lastLedStatus = 0;
// =====================================================
// PSRAM LOADER
// =====================================================

static bool loadFileToPSRAM(const char *path, uint8_t **buffer, size_t *size)
{
  File f = LittleFS.open(path, "r");
  if (!f)
  {
    Serial.printf("[PSRAM] Open failed %s\n", path);
    return false;
  }

  *size = f.size();

  *buffer =
      (uint8_t *)heap_caps_malloc(*size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!*buffer)
  {
    Serial.printf("[PSRAM] malloc failed %s (%u)\n", path, *size);
    f.close();
    return false;
  }

  f.read(*buffer, *size);
  f.close();

  Serial.printf("[PSRAM] Cached %s (%u bytes)\n", path, *size);

  return true;
}

// =====================================================
// PRELOAD
// =====================================================

static void preloadImagesToPSRAM()
{
  Serial.println("[PSRAM] Preloading images...");

  for (int i = 0; i < BONGO_COUNT; i++)
  {
    loadFileToPSRAM(BONGO_IMAGES[i], &bongoCache[i], &bongoSize[i]);
  }

  loadFileToPSRAM("/icons/con.jpg", &btIconConnected, &btIconConnectedSize);

  loadFileToPSRAM("/icons/dis.jpg", &btIconDisconnected,
                  &btIconDisconnectedSize);

  loadFileToPSRAM("/icons/caps.jpg", &capsIcon, &capsIconSize);

  Serial.println("[PSRAM] Image preload done");
}

// =====================================================
// JPEG DRAW
// =====================================================

void drawStatusBar(bool isConnected, int batteryPercent, uint8_t ledStatus);

static void drawJpegFromPSRAM(int x, int y, uint8_t *buf, size_t len)
{
  if (!buf || !len)
    return;

  if (!xSemaphoreTake(tftMutex, portMAX_DELAY))
    return;
  fex.drawJpeg(buf, len, x, y);

  xSemaphoreGive(tftMutex);
}

void displayJPEG(const char *filename, int x, int y)
{
  for (int i = 0; i < BONGO_COUNT; i++)
  {
    if (!strcmp(filename, BONGO_IMAGES[i]) && bongoCache[i])
    {

      drawJpegFromPSRAM(x, y, bongoCache[i], bongoSize[i]);
      return;
    }
  }

  if (!strcmp(filename, "/icons/con.jpg") && btIconConnected)
  {

    drawJpegFromPSRAM(x, y, btIconConnected, btIconConnectedSize);
    return;
  }

  if (!strcmp(filename, "/icons/dis.jpg") && btIconDisconnected)
  {

    drawJpegFromPSRAM(x, y, btIconDisconnected, btIconDisconnectedSize);
    return;
  }

    if (!strcmp(filename, "/icons/caps.jpg") && capsIcon)
  {

    drawJpegFromPSRAM(x, y, capsIcon, capsIconSize);
    return;
  }

  // fallback SPIFFS
  if (xSemaphoreTake(tftMutex, portMAX_DELAY))
  {
    fex.drawJpeg(filename, x, y);
    xSemaphoreGive(tftMutex);
  }
}

// =====================================================
// DISPLAY TASK
// =====================================================

static void keyDisplayTask(void *parameter)
{
  lastActivityTime = millis();

  ImageRequest imgReq;
  StatusBarRequest statusReq;

  while (1)
  {

    // ---------- STATUS BAR ----------
    if (statusBarQueue &&
        xQueueReceive(statusBarQueue, &statusReq, pdMS_TO_TICKS(50)))
    {
      if (statusReq.isConnected != lastConnectionState ||
          statusReq.batteryPercent != lastBatteryPercent ||
          statusReq.ledStatus != lastLedStatus)
      {
        lastConnectionState = statusReq.isConnected;
        lastBatteryPercent = statusReq.batteryPercent;
        lastLedStatus = statusReq.ledStatus;

        drawStatusBar(statusReq.isConnected, statusReq.batteryPercent, statusReq.ledStatus);
      }
      continue;
    }

    // ---------- IMAGE ----------
    if (imageQueue &&
        xQueueReceive(imageQueue, &imgReq, pdMS_TO_TICKS(KEY_IDLE_TIMEOUT)))
    {
      lastKeyTime = millis();
      lastActivityTime = millis();

      digitalWrite(TFT_BL, HIGH);

      if (currentImage != imgReq.imageIndex)
      {
        currentImage = imgReq.imageIndex;
        displayJPEG(BONGO_IMAGES[currentImage], 0,
                    STATUSBAR_HEIGHT + IMAGE_PADDING);
      }

      if (xSemaphoreTake(tftMutex, pdMS_TO_TICKS(50)))
      {
        if (imgReq.keyChar)
        {
          tft.setTextSize(10);
          tft.setTextColor(TFT_DARKGREY);
          tft.fillRect(tft.width() / 2 - 20, STATUSBAR_HEIGHT + 40, 60, 60,
                       TFT_WHITE);
          tft.setCursor(tft.width() / 2 - 20, STATUSBAR_HEIGHT + 40);
          tft.print(imgReq.keyChar);
        }
        xSemaphoreGive(tftMutex);
      }
    }

    // ---------- IDLE ----------
    if (xSemaphoreTake(keysMutex, 10))
    {
      int copy = keysPressed;
      xSemaphoreGive(keysMutex);

      if (copy == 0 && millis() - lastKeyTime > KEY_IDLE_TIMEOUT)
      {
        if (currentImage != 0)
        {
          currentImage = 0;
          tft.fillRect(tft.width() / 2 - 20, STATUSBAR_HEIGHT + 40, 60, 60,
                       TFT_WHITE);
          displayJPEG(BONGO_IMAGES[0], 0, STATUSBAR_HEIGHT + IMAGE_PADDING);
        }
      }
    }

    // ---------- BACKLIGHT ----------
    if (millis() - lastActivityTime > INACTIVITY_TIMEOUT)
    {

      digitalWrite(TFT_BL, LOW);
      lastActivityTime = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

// =====================================================
// STATUS BAR
// =====================================================

void drawStatusBar(bool isConnected, int batteryPercent, uint8_t ledStatus)
{
  const char *btIcon = isConnected ? "/icons/con.jpg" : "/icons/dis.jpg";

  if (!xSemaphoreTake(tftMutex, portMAX_DELAY))
    return;

  tft.fillRect(0, 0, 50, STATUSBAR_HEIGHT, TFT_WHITE);
  xSemaphoreGive(tftMutex);
  displayJPEG(btIcon, 15, 5);
  if (!xSemaphoreTake(tftMutex, portMAX_DELAY))
    return;

  if (ledStatus != 0xff)
  {
    tft.fillRect(50 + 10, 0, 30, STATUSBAR_HEIGHT, TFT_WHITE);
    if (ledStatus & 0x02)
    {
      xSemaphoreGive(tftMutex);
      displayJPEG("/icons/caps.jpg", 50 + 15, 5);
      if (!xSemaphoreTake(tftMutex, portMAX_DELAY))
    return;
    }
  }
  if (batteryPercent != -1)
  {
    tft.fillRect(tft.width() - 50, 0, 50, STATUSBAR_HEIGHT, TFT_WHITE);
    if (batteryPercent < 0)
      batteryPercent = 0;
    if (batteryPercent > 100)
      batteryPercent = 100;

    const int battW = 40;
    const int battH = 22;
    const int battX = tft.width() - battW - 15;
    const int battY = 8;

    tft.drawRoundRect(battX, battY, battW, battH, 4, TFT_BLACK);

    tft.fillRect(battX - 4, battY + battH / 4, 4, battH / 2, TFT_BLACK);

    int fillW = (battW - 4) * batteryPercent / 100;

    uint16_t fillColor = TFT_GREEN;
    if (batteryPercent <= 20)
      fillColor = TFT_RED;
    else if (batteryPercent <= 50)
      fillColor = TFT_ORANGE;

    tft.fillRoundRect(battX + 2, battY + 2, fillW, battH - 4, 3, fillColor);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString(String(batteryPercent) + "%", battX + battW / 2,
                   battY + battH / 2);

    tft.setTextDatum(TL_DATUM);
  }
  xSemaphoreGive(tftMutex);
}

// =====================================================
// INIT / PUBLIC API
// =====================================================

void displayInit()
{
  tftMutex = xSemaphoreCreateMutex();
  keysMutex = xSemaphoreCreateMutex();
  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(2);
  LittleFS.begin();
  preloadImagesToPSRAM();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  delay(200);
}

void displayStartKeyMonitor()
{
  imageQueue = xQueueCreate(10, sizeof(ImageRequest));
  statusBarQueue = xQueueCreate(5, sizeof(StatusBarRequest));

  lastKeyTime = millis();
  lastActivityTime = millis();

  xTaskCreatePinnedToCore(keyDisplayTask, "KeyDisplayTask", 6144, nullptr, 2,
                          nullptr, 0);
}

void displayUpdateStatus(bool isConnected, int batteryPercent, uint8_t ledStatus = 0)
{
  if (!statusBarQueue)
    return;

  StatusBarRequest r{isConnected, batteryPercent, ledStatus};

  xQueueSend(statusBarQueue, &r, 0);
}

void displayKeyPressed(char key)
{
  int count;

  xSemaphoreTake(keysMutex, portMAX_DELAY);
  keysPressed++;
  count = keysPressed;
  xSemaphoreGive(keysMutex);

  ImageRequest req;
  req.keyChar = key;

  req.imageIndex = (count == 1) ? 2 + (rand() % 6) : 1;

  if (imageQueue)
    xQueueSend(imageQueue, &req, 0);
}

void displayKeyReleased()
{
  int count;

  xSemaphoreTake(keysMutex, portMAX_DELAY);
  if (keysPressed > 0)
    keysPressed--;
  count = keysPressed;
  xSemaphoreGive(keysMutex);

  if (count == 1 && imageQueue)
  {
    ImageRequest req;
    req.imageIndex = 2 + (rand() % 6);
    req.keyChar = '\0';
    xQueueSend(imageQueue, &req, 0);
  }
}

void displayClearScreen()
{
  if (xSemaphoreTake(tftMutex, portMAX_DELAY))
  {
    tft.fillScreen(TFT_WHITE);
    xSemaphoreGive(tftMutex);
  }
}
