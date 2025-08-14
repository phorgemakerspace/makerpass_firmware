// WiFi management functions for MakerPass firmware
// This module handles WiFi connection and reconnection logic

#include "wifi_manager.h"
#include "pins.h"
#include "constants.h"
#include "config.h"
#include "ui_manager.h"

extern bool wifiConnected;
extern bool authenticated;
extern bool wsConnected;
extern TFT_eSPI tft;

// Attempt to connect to the configured WiFi network.  While connecting
// the WiFi status LED blinks at ~1 Hz.  On success the LED remains
// lit; on failure it is turned off.
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  showBootMessage("Connecting WiFi");

  unsigned long start = millis();
  bool ledState = false;
  while (WiFi.status() != WL_CONNECTED && (millis() - start < 20000)) {
    // blink WiFi LED while trying to connect
    digitalWrite(PIN_LED_WIFI, ledState);
    ledState = !ledState;
    delay(500);
  }
  // ensure LED off during update
  digitalWrite(PIN_LED_WIFI, LOW);

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(PIN_LED_WIFI, HIGH);
    showBootMessage("WiFi Connected", WiFi.localIP().toString());
  } else {
    wifiConnected = false;
    showBootMessage("WiFi Failed", "", COLOR_MSG_ERR);
  }
  delay(1000);
}

// Check WiFi status and handle reconnection
void handleWiFiStatus() {
  wl_status_t wifiStatus = WiFi.status();
  if (wifiStatus == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      digitalWrite(PIN_LED_WIFI, HIGH);
      showStatusBar();
    }
  } else {
    if (wifiConnected) {
      wifiConnected = false;
      authenticated = false;
      wsConnected = false;
      digitalWrite(PIN_LED_WIFI, LOW);
      showStatusBar();
      showMessage("Offline", "Master Key Only", COLOR_MSG_WARN);
    }
    // Attempt to reconnect WiFi periodically
    static unsigned long lastWifiRetry = 0;
    if (millis() - lastWifiRetry > 10000) {
      Serial.println(F("[WiFi] Attempting reconnect"));
      WiFi.disconnect();
      WiFi.reconnect();
      lastWifiRetry = millis();
    }
  }
}
