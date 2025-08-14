// Display configuration and constants for the MakerPass firmware
// This header defines screen dimensions, colors, and UI constants

#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// ---------------------------------------------------------------------------
// Display constants
// ---------------------------------------------------------------------------

// With rotation 3 the width/height are swapped; width = 320, height = 170.
static const uint16_t SCREEN_WIDTH  = 320;
static const uint16_t SCREEN_HEIGHT = 170;

// Layout dimensions
static const uint16_t TOP_STATUS_BAR_H    = 36;  // Increased height for larger top padding
static const uint16_t BOTTOM_STATUS_BAR_H = 20;  // Bottom bar for connection status
static const uint16_t MESSAGE_AREA_Y      = TOP_STATUS_BAR_H;
static const uint16_t MESSAGE_AREA_H      = SCREEN_HEIGHT - TOP_STATUS_BAR_H - BOTTOM_STATUS_BAR_H;

// Colours used by the UI (16‑bit 565 format).  The TFT_eSPI library
// defines a palette of colours such as TFT_BLACK and TFT_WHITE.  We
// create a few more for convenience.
static const uint16_t COLOR_BG        = TFT_BLACK;
static const uint16_t COLOR_STATUS_BG = TFT_DARKGREY;
static const uint16_t COLOR_STATUS_TX = TFT_WHITE;
static const uint16_t COLOR_MSG_OK    = TFT_GREEN;
static const uint16_t COLOR_MSG_WARN  = TFT_YELLOW;
static const uint16_t COLOR_MSG_ERR   = TFT_RED;

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------

// Ping/pong keep‑alive - Server sends pings every 5 minutes, 15-minute timeout
static const unsigned long PING_INTERVAL_MS = 300000; // 5 minutes (but server initiates pings)
static const unsigned long PONG_TIMEOUT_MS  = 960000; // 16 minutes (slightly longer than server's 15-min timeout)

// Card presence tracking for require_card_present
static const unsigned long CARD_PRESENT_TIMEOUT_MS = 2000; // treat card as removed after 2 s
