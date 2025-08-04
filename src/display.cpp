/*
 * MakerPass Firmware
 * Display Functions - TFT display management
 */

#include "makerpass.h"

void setupDisplay() {
  // Manual reset sequence
  pinMode(13, OUTPUT);  // Reset pin
  pinMode(14, OUTPUT);  // DC pin
  
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  
  // Initialize
  tft.init();
  tft.setRotation(3);  // Landscape Inverted (320x170)
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("Display initialized successfully");
}

void showMessage(String line1, String line2, String line3, uint16_t color, int duration) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(color);
  tft.setTextFont(4);
  tft.setTextSize(1);
  
  // Center text on 320x170 display (landscape)
  int x = 20;
  
  tft.setCursor(x, 30);
  tft.print(line1);
  
  tft.setCursor(x, 70);
  tft.print(line2);
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setCursor(x, 120);
  tft.print(line3);
  
  if (duration == 0) {
    // Permanent message
    messageActive = false;
    messageExpireTime = 0;
  } else if (duration <= 500) {
    // Short message with blocking delay
    delay(duration);
    showMainScreen();
    messageActive = false;
    messageExpireTime = 0;
  } else {
    // Long message with non-blocking timing
    messageActive = true;
    messageExpireTime = millis() + duration;
  }
}

void showMainScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(4);
  tft.setTextSize(1);
  
  // Display device name at top
  tft.setCursor(10, 10);
  if (resourceName.length() > 0) {
    tft.print(resourceName);
  } else if (systemReady) {
    tft.print("MakerPass Device");
  } else {
    tft.print("Master Key Mode");
  }
  
  // Display main message with different text/color based on system state
  tft.setCursor(10, 65);
  tft.setTextFont(4);
  
  if (!systemReady) {
    tft.setTextColor(TFT_BLUE);
    tft.print("System Offline.");
    tft.setCursor(10, 100);
    tft.print("Scan Master Key.");
  } else if (!isWifiConnected) {
    tft.setTextColor(TFT_BLUE);
    tft.print("WiFi Unavailable.");
    tft.setCursor(10, 100);
    tft.print("Scan Master Key.");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.print("Ready. Scan card.");
  }
  
  // WiFi status at bottom
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setCursor(10, 145);
  tft.print("WiFi: ");
  if (isWifiConnected) {
    tft.setTextColor(TFT_GREEN);
    tft.print("Connected");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("Disconnected");
  }
}

void updateDisplay() {
  static bool lastRelayState = false;
  static String lastActiveUser = "";
  static unsigned long lastRemainingTime = 0;

  // Force a full refresh if the relay state or active user changes
  bool needFullRefresh = (relayActive != lastRelayState) || (activeUser != lastActiveUser);

  // If relay just turned off, show main screen ONLY if no message is active
  if (!relayActive && lastRelayState) {
    lastRelayState = relayActive;
    lastActiveUser = activeUser;
    if (!messageActive) {
      showMainScreen();
    }
    return;
  }

  // Only proceed if relay is active
  if (!relayActive) {
    return;
  }

  if (needFullRefresh) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(4);
    tft.setTextSize(1);

    // Display resource name at top
    tft.setCursor(10, 10);
    if (resourceName.length() > 0) {
      tft.print(resourceName);
    } else if (systemReady) {
      tft.print("MakerPass Device");
    } else {
      tft.print("Master Key Mode");
    }

    // Display status
    tft.setCursor(10, 45);
    tft.setTextColor(TFT_GREEN);
    tft.print("Active");
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setCursor(10, 75);
    tft.print("User: ");
    tft.print(activeUser);

    // WiFi status at bottom
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setCursor(10, 145);
    tft.print("WiFi: ");
    if (isWifiConnected) {
      tft.setTextColor(TFT_GREEN);
      tft.print("Connected");
    } else {
      tft.setTextColor(TFT_RED);
      tft.print("Disconnected");
    }
  }

  // Update runtime for machines dynamically
  if (relayActive && !isDoor) {
    // Clear just the runtime area
    tft.fillRect(10, 100, 300, 40, TFT_BLACK);

    // Show runtime
    unsigned long runTime = (millis() - relayStartTime) / 1000;
    unsigned long hours = runTime / 3600;
    unsigned long minutes = (runTime % 3600) / 60;
    unsigned long seconds = runTime % 60;

    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setCursor(10, 105);
    tft.print("Runtime: ");
    if (hours < 10) tft.print("0");
    tft.print(hours);
    tft.print(":");
    if (minutes < 10) tft.print("0");
    tft.print(minutes);
    tft.print(":");
    if (seconds < 10) tft.print("0");
    tft.print(seconds);
  }

  // Update countdown for doors dynamically
  if (relayActive && isDoor && systemReady) {
    unsigned long remainingTime = (doorOpenTime - (millis() - relayStartTime)) / 1000;
    if (remainingTime != lastRemainingTime) {
      tft.fillRect(10, 100, 300, 40, TFT_BLACK);
      tft.setCursor(10, 105);
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(2);
      tft.print("Door closes in: ");
      tft.print(remainingTime);
      tft.print("s");
      lastRemainingTime = remainingTime;
    }
  }

  // Update static variables
  lastRelayState = relayActive;
  lastActiveUser = activeUser;
}
