// Session management header for MakerPass firmware

#pragma once

#include <Arduino.h>

// Function declarations
void flashRFIDIndicator(uint16_t durationMs = 100);
void unlockRelay(const String &userName);
void lockRelay();
void startSession(const String &sessionId, const String &userName);
void endSession(const String &userName);
