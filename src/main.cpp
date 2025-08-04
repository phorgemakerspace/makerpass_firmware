/*
 * MakerPass Firmware
 * 
 * This firmware is designed for ESP32-based MakerPass hardware to control access
 * to machines and doors using an RFID reader, interfacing with the MakerPass
 * plugin for Moodle.
 */

#include "makerpass.h"

// Hardware objects
TFT_eSPI tft = TFT_eSPI();
WIEGAND wiegand;

// Global variables
String apiKey;
String apiUrl;
String equipmentId;
String resourceName;
bool isDoor;
bool cardPresentRequired;
unsigned long doorOpenTime;
unsigned long lastCardTime = 0;
bool relayActive = false;
unsigned long relayStartTime = 0;
String activeUser = "";
bool isWifiConnected = false;
bool systemReady = false;

// Message display timing
unsigned long messageExpireTime = 0;
bool messageActive = false;

void setup() {
  // Set boot pins to safe states
  pinMode(0, INPUT_PULLUP);  // GPIO0 - boot pin
  pinMode(2, INPUT_PULLUP);  // GPIO2 - boot pin
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("");
  Serial.println("=== MAKERPASS SYSTEM STARTUP ===");
  
  // Initialize hardware components
  initializeHardware();
  
  // Initialize system configuration and networking
  initializeSystem();
}

void loop() {
  runMainLoop();
}



