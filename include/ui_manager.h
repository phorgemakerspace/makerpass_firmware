// UI management header for MakerPass firmware

#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "constants.h"

// Function declarations
void getTextDimensions(const String &text, uint8_t font, uint16_t &width, uint16_t &height);
void showStatusBar();
void showTopStatusBar();
void showBottomStatusBar();
void showMessage(const String &line1, const String &line2 = "", uint16_t textColor = COLOR_STATUS_TX, uint16_t bgColor = COLOR_BG);
void showTempMessage(const String &line1, const String &line2 = "", uint16_t textColor = COLOR_STATUS_TX, uint16_t bgColor = COLOR_BG);
void showBootMessage(const String &message, const String &detail = "", uint16_t textColor = TFT_WHITE);
void showIdleScreen();
void showRuntimeDisplay(const String &userName, const String &runtime, bool initialDraw = false);
void showDoorCountdown(const String &header, const String &seconds, bool initialDraw = false);
void resetRuntimeDisplay();
