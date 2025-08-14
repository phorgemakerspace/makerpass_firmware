// UI management functions for MakerPass firmware
// This module handles all display and user interface operations

#include "ui_manager.h"
#include "constants.h"

extern TFT_eSPI tft;
extern String resourceName;
extern bool wifiConnected;
extern bool wsConnected;
extern bool authenticated;


// Draw the top status bar
void showTopStatusBar() {
  // Clear the top area with dark gray background to match bottom status bar
  tft.fillRect(0, 0, SCREEN_WIDTH, TOP_STATUS_BAR_H, 0x1082); // Very dark gray, barely lighter than black
  
  // Device name in white
  tft.setTextFont(4);
  tft.setTextColor(TFT_WHITE, 0x1082);
  
  String deviceText;
  // Use resourceName if available, otherwise default to "MakerPass Device"
  if (resourceName.length() > 0) {
    deviceText = resourceName;
  } else {
    deviceText = "MakerPass Device";
  }

  tft.setCursor(10, 8); 
  tft.print(deviceText);
}

// Draw the bottom status bar with connection indicators
void showBottomStatusBar() {
  int bottomY = SCREEN_HEIGHT - BOTTOM_STATUS_BAR_H;
  tft.fillRect(0, bottomY, SCREEN_WIDTH, BOTTOM_STATUS_BAR_H, 0x1082);
  
  tft.setTextFont(2);
  tft.setTextColor(COLOR_STATUS_TX, 0x1082);
  
  // WiFi status with dot
  tft.setCursor(10, bottomY + 2);
  tft.print("WiFi");
  tft.fillCircle(50, bottomY + 8, 4, wifiConnected ? TFT_GREEN : 0xF800);
  
  // Server status with dot
  tft.setCursor(70, bottomY + 2);
  tft.print("Server");
  tft.fillCircle(120, bottomY + 8, 4, authenticated ? TFT_GREEN : 0xF800);
}

// Update both status bars
void showStatusBar() {
  showTopStatusBar();
  showBottomStatusBar();
}

// Display a multi‑line message in the main message area between status bars
void showMessage(const String &line1, const String &line2, uint16_t textColor, uint16_t bgColor) {
  // Clear the message area (between the two status bars)
  tft.fillRect(0, MESSAGE_AREA_Y, SCREEN_WIDTH, MESSAGE_AREA_H, bgColor);
  tft.setTextColor(textColor, bgColor);
  
  // Use a large font for the first line
  tft.setTextFont(4);
  tft.setCursor(10, MESSAGE_AREA_Y + 35);
  tft.print(line1);
  
  // Second line in smaller font below first line
  if (line2.length() > 0) {
    tft.setTextFont(2);
    tft.setCursor(10, MESSAGE_AREA_Y + 70);
    tft.print(line2);
  }
  
  // Always show status bars
  showStatusBar();
}

// Display a temporary message that auto-clears after 3 seconds
void showTempMessage(const String &line1, const String &line2, uint16_t textColor, uint16_t bgColor) {
  showMessage(line1, line2, textColor, bgColor);
  
  // Wait 3 seconds then show idle screen
  delay(3000);
  showIdleScreen();
}

// Show boot-time messages with simpler formatting
void showBootMessage(const String &message, const String &detail, uint16_t textColor) {
  tft.fillScreen(COLOR_BG);
  tft.setCursor(10, SCREEN_HEIGHT / 2 - 20);
  tft.setTextFont(4);
  tft.setTextColor(textColor, COLOR_BG);
  tft.println(message);
  if (detail.length() > 0) {
    tft.setTextFont(2);
    tft.println(detail);
  }
}

// Show the idle screen when device is ready
void showIdleScreen() {
  if (authenticated) {
    showMessage("Ready", "Scan card", COLOR_MSG_OK);
  } else {
    showMessage("Offline", "Master Key Only", COLOR_MSG_WARN);
  }
}

// Show runtime display
void showRuntimeDisplay(const String &userName, const String &runtime, bool initialDraw) {
  static String lastRuntime = "";
  
  if (initialDraw) {
    // Full redraw - clear message area and draw everything
    tft.fillRect(0, MESSAGE_AREA_Y, SCREEN_WIDTH, MESSAGE_AREA_H, COLOR_BG);
    
    // User name
    tft.setTextFont(4);
    tft.setTextColor(COLOR_MSG_OK, COLOR_BG);
    tft.setCursor(10, MESSAGE_AREA_Y + 35);
    tft.print(userName);
    
    // Runtime label and time in smaller font
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, COLOR_BG);
    tft.setCursor(10, MESSAGE_AREA_Y + 70); // Below user name
    tft.print("Runtime: ");
    tft.print(runtime);
    
    // Show status bars
    showStatusBar();
    lastRuntime = runtime;
  } else if (runtime != lastRuntime) {
    // Efficient update - only update the time portion
    tft.setTextFont(2);
    tft.setTextColor(COLOR_MSG_OK, COLOR_BG);
    
    // Clear just the time area (approximate width for HH:MM:SS)
    tft.fillRect(65, MESSAGE_AREA_Y + 70, 80, 16, COLOR_BG); // Clear time area
    
    // Redraw just the time
    tft.setCursor(65, MESSAGE_AREA_Y + 70); // After "Runtime: "
    tft.print(runtime);
    lastRuntime = runtime;
  }
}

// Reset runtime display state for new sessions
void resetRuntimeDisplay() {
  // This will be used in main.cpp to reset the firstDraw flag
}

// Show a door countdown screen with efficient time-only updates
void showDoorCountdown(const String &header, const String &seconds, bool initialDraw) {
  static String lastSeconds = "";
  static int16_t secondsX = -1;
  static int16_t secondsY = -1;

  if (initialDraw) {
    // Full redraw of message area
    tft.fillRect(0, MESSAGE_AREA_Y, SCREEN_WIDTH, MESSAGE_AREA_H, COLOR_BG);

    // Header (e.g., "Access Granted") in large font, left-justified
    tft.setTextFont(4);
    tft.setTextColor(COLOR_MSG_OK, COLOR_BG);
    tft.setCursor(10, MESSAGE_AREA_Y + 35);
    tft.print(header);

    // Label + seconds in smaller font; label in white, seconds in white too
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, COLOR_BG);
    secondsY = MESSAGE_AREA_Y + 70;

    // Draw the label and compute where the seconds should start
    const char* label = "Locking in: ";
    tft.setCursor(10, secondsY);
    tft.print(label);
    int16_t labelWidth = tft.textWidth(label); // current font is 2
    secondsX = 10 + labelWidth;

    // Draw the initial seconds exactly at computed X
    tft.setCursor(secondsX, secondsY);
    tft.print(seconds);

    showStatusBar();
    lastSeconds = seconds;
  } else if (seconds != lastSeconds) {
    // Only update the seconds text at the exact same X position
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, COLOR_BG);
    if (secondsX < 0) {
      // Fallback: compute based on label width if not initialized
      const char* label = "Locking in: ";
      int16_t labelWidth = tft.textWidth(label);
      secondsX = 10 + labelWidth;
      secondsY = MESSAGE_AREA_Y + 70;
    }
    // Clear precisely the previous seconds width (with a small padding)
    int16_t oldW = tft.textWidth(lastSeconds);
    int16_t newW = tft.textWidth(seconds);
    int16_t clearW = (oldW > newW ? oldW : newW) + 6;
    tft.fillRect(secondsX, secondsY, clearW, 16, COLOR_BG);

    // Redraw the new seconds string
    tft.setCursor(secondsX, secondsY);
    tft.print(seconds);
    lastSeconds = seconds;
  }
}
