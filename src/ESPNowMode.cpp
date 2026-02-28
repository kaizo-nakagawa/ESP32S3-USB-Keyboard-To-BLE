#include "ESPNowMode.h"
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

ESPNowMode::ESPNowMode(const uint8_t targetMacAddress[6]) {
  memcpy(this->targetMacAddress, targetMacAddress, 6);
}

void ESPNowMode::begin() {
  // Initialize WiFi for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) return;

  // Register ESP-NOW peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, targetMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void ESPNowMode::sendKeyboard(const uint8_t *keys, uint8_t modifier) {
  EspNowKeyboardReport espNowReport;
  espNowReport.modifier = modifier;
  memcpy(espNowReport.keys, keys, 6);
  esp_now_send(targetMacAddress, (uint8_t *)&espNowReport, sizeof(espNowReport));
}

void ESPNowMode::sendMouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) {
  // ESP-NOW mouse report: [buttons, x, y, wheel]
  uint8_t mouseReport[4] = {buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel};
  esp_now_send(targetMacAddress, mouseReport, sizeof(mouseReport));
}

void ESPNowMode::sendMedia(uint8_t consumerCode) {
  // Send as raw 2-byte: type + code
  uint8_t mediaReport[2] = {0x02, consumerCode};  // 0x02 = media type
  esp_now_send(targetMacAddress, mediaReport, 2);
}

void ESPNowMode::sendJoystick(uint8_t buttons, uint8_t x, uint8_t y, uint8_t z) {
  // Joystick report: [buttons, x, y, z]
  uint8_t joystickReport[4] = {buttons, x, y, z};
  esp_now_send(targetMacAddress, joystickReport, sizeof(joystickReport));
}

bool ESPNowMode::isConnected() {
  // ESP-NOW doesn't have a connection state, always assume connected
  return true;
}

std::string ESPNowMode::getConnectedName() {
  // Return MAC address as string
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           targetMacAddress[0], targetMacAddress[1], targetMacAddress[2],
           targetMacAddress[3], targetMacAddress[4], targetMacAddress[5]);
  return std::string(macStr);
}

void ESPNowMode::reportBatteryLevel(int percentage) {
  // ESP-NOW doesn't have native battery reporting, could be implemented via custom protocol
  // For now, this is a no-op
}
