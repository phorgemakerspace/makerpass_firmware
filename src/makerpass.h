/*
 * MakerPass Firmware
 * Header File - Main declarations and constants
 */

#ifndef MAKERPASS_H
#define MAKERPASS_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "Wiegand.h"
#include "config.h"

// Pin Definitions
#define LED_WIFI        27  // WiFi status LED
#define LED_RELAY       26  // Relay status LED
#define LED_RFID        25  // RFID status LED
#define RFID_D0         4   // Wiegand D0
#define RFID_D1         5   // Wiegand D1
#define RFID_LED        18  // RFID module LED
#define RFID_BEEP       19  // RFID module beeper
#define RELAY_PIN       23  // Main relay control

// Default constants (can be overridden by config.cpp)
const unsigned long DEFAULT_DOOR_OPEN_TIME = 5000;

// Global variables
extern String apiKey;
extern String apiUrl;
extern String equipmentId;
extern String resourceName;
extern bool isDoor;
extern bool cardPresentRequired;
extern unsigned long doorOpenTime;
extern unsigned long lastCardTime;
extern bool relayActive;
extern unsigned long relayStartTime;
extern String activeUser;
extern bool isWifiConnected;
extern bool systemReady;

// Message display timing
extern unsigned long messageExpireTime;
extern bool messageActive;

// Hardware objects
extern TFT_eSPI tft;
extern WIEGAND wiegand;

// Function declarations
void initializeHardware();
void initializeSystem();
void runMainLoop();

// Network functions
bool connectToWifi();
bool retrieveResourceConfig();
bool checkAccess(String rfidCode);

// Hardware functions
void setupDisplay();
void setupRFID();
void controlRelay(bool state);

// RFID and access control
void handleRFID(uint32_t cardId);

// Display functions
void updateDisplay();
void showMainScreen();
void showMessage(String line1, String line2, String line3, uint16_t color, int duration);

#endif
