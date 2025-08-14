// Pin assignments for the MakerPass firmware
// This header defines all the GPIO pins used by the hardware

#pragma once

#include <Arduino.h>

// Wiegand data pins
static const uint8_t PIN_RFID_D0   = 4;
static const uint8_t PIN_RFID_D1   = 5;

// Control lines for reader LED and beeper
static const uint8_t PIN_RFID_LED  = 18;
static const uint8_t PIN_RFID_BEEP = 19;

// Relay output controlling the door or machine
static const uint8_t PIN_RELAY     = 23;

// Status LEDs: WiFi, relay active and RFID activity
static const uint8_t PIN_LED_WIFI  = 27;
static const uint8_t PIN_LED_RELAY = 26;
static const uint8_t PIN_LED_RFID  = 25;

// Display pins (also configured in platformio.ini)
// static const uint8_t PIN_TFT_MOSI = 21;
// static const uint8_t PIN_TFT_SCLK = 22;
// static const uint8_t PIN_TFT_DC   = 14;
// static const uint8_t PIN_TFT_RST  = 13;
