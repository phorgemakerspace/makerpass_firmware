/*
 * MakerPass WebSocket firmware with RFID (Wiegand) access control
 *
 * This firmware runs on a custom ESP32‑WROVER‑IE based PCB equipped
 * with a 1.9″ ST7789 TFT display and a Wiegand RFID reader. It
 * connects to the MakerPass server over WiFi using a WebSocket to
 * authenticate itself and to relay RFID scans to the server. The
 * server authorizes access and instructs the device to power a relay
 * controlling a door or machine. A master RFID card can be used
 * to unlock the relay when the network is unavailable.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "Wiegand.h"

#include "config.h"
#include "pins.h"
#include "constants.h"
#include "ui_manager.h"
#include "wifi_manager.h"
#include "websocket_manager.h"
#include "session_manager.h"

// ---------------------------------------------------------------------------
// Global objects and state
// ---------------------------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();          // Display driver instance
WIEGAND wiegand;                    // RFID reader interface
WebSocketsClient webSocket;         // WebSocket client

// Connection flags
bool wifiConnected      = false;    // true when WiFi is associated
bool wsConnected        = false;    // true when WebSocket is open
bool authenticated      = false;    // true when auth_success has been received
bool resourceEnabled    = false;    // value from auth_success
bool requireCardPresent = false;    // value from auth_success

// Resource name reported by the server
String resourceName = "MakerPass";

// Session management
String currentSessionId = "";       // non‑empty when a session is active
String activeUser       = "";       // user currently granted access
unsigned long sessionStartTime = 0; // for machines: when the session started
bool runtimeDisplayReset = true;    // flag to trigger full runtime display redraw

// Relay timing for door mode
unsigned long relayEndTime = 0;     // millis when the relay should be turned off
bool relayActive = false;           // true while the relay is energised

// RFID indicator timing
unsigned long rfidIndicatorEndTime = 0; // when to turn off the RFID LED/beep

// Ping/pong keep‑alive
unsigned long lastPingTime = 0;
unsigned long lastPongTime = 0;

// Card presence tracking for require_card_present
String lastCardCode = "";
unsigned long lastCardTime = 0;

// Forward declarations for functions local to main.cpp
void updateTimers();
void checkCardPresence();
void handleRFIDScan();

// ---------------------------------------------------------------------------
// Setup: configure hardware, connect to WiFi and initialise subsystems
// ---------------------------------------------------------------------------

void setup() {
  // Start the serial port for debugging
  Serial.begin(115200);
  delay(100);

  // Configure GPIO pins
  pinMode(PIN_RFID_D0, INPUT_PULLUP);
  pinMode(PIN_RFID_D1, INPUT_PULLUP);

  pinMode(PIN_RFID_LED, OUTPUT);
  pinMode(PIN_RFID_BEEP, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_WIFI, OUTPUT);
  pinMode(PIN_LED_RELAY, OUTPUT);
  pinMode(PIN_LED_RFID, OUTPUT);

  // Initialise outputs to a safe state
  digitalWrite(PIN_RFID_LED, LOW);
  digitalWrite(PIN_RFID_BEEP, LOW);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED_WIFI, LOW);
  digitalWrite(PIN_LED_RELAY, LOW);
  digitalWrite(PIN_LED_RFID, LOW);

  // Brief LED test: turn on all status LEDs sequentially
  digitalWrite(PIN_LED_WIFI, HIGH);
  delay(100);
  digitalWrite(PIN_LED_RELAY, HIGH);
  delay(100);
  digitalWrite(PIN_LED_RFID, HIGH);
  delay(100);
  digitalWrite(PIN_LED_WIFI, LOW);
  digitalWrite(PIN_LED_RELAY, LOW);
  digitalWrite(PIN_LED_RFID, LOW);

   // Manual display reset sequence
  pinMode(13, OUTPUT);  // Reset pin
  pinMode(14, OUTPUT);  // DC pin
  
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);

  // Initialise the TFT display
  tft.init();
  tft.setRotation(3); // landscape orientation
  tft.fillScreen(COLOR_BG);
  showBootMessage("MakerPass Booting...");

  // Initialise the Wiegand RFID reader.  The library uses
  // interrupts internally; pinMode has already configured the
  // inputs with pull‑ups.  Begin must be called after pinMode.
  wiegand.begin(PIN_RFID_D0, PIN_RFID_D1);

  // Flash the reader's LED and beeper briefly to indicate readiness
  digitalWrite(PIN_RFID_LED, HIGH);
  digitalWrite(PIN_RFID_BEEP, HIGH);
  delay(100);
  digitalWrite(PIN_RFID_LED, LOW);
  digitalWrite(PIN_RFID_BEEP, LOW);

  // Connect to WiFi.  This function blocks until either a
  // connection is established or a timeout expires.
  connectToWiFi();

  // Initialise the WebSocket client
  initWebSocket();
  
  // Prepare the status bar for subsequent screens
  showStatusBar();
}

// ---------------------------------------------------------------------------
// Main loop: handle events, scan cards and update timers
// ---------------------------------------------------------------------------

void loop() {
  // Maintain the WebSocket connection and process incoming frames
  webSocket.loop();

  // Send periodic pings to keep the connection alive
  handleWebSocketKeepAlive();

  // Check WiFi connection state and update the status bar if it changes
  handleWiFiStatus();

  // Check for new RFID cards
  handleRFIDScan();

  // Update timers and UI (relay countdown or session runtime)
  updateTimers();

  // Monitor card presence and end session if required
  checkCardPresence();
}

// ---------------------------------------------------------------------------
// RFID handling
// ---------------------------------------------------------------------------

// Handle RFID card scans
void handleRFIDScan() {
  if (wiegand.available()) {
    uint32_t code = wiegand.getCode();
    char buf[9];
    snprintf(buf, sizeof(buf), "%08X", code);
    String codeStr = String(buf);
    Serial.print(F("[RFID] Scanned card: 0x"));
    Serial.println(codeStr);
    
    // Record last card for presence detection
    lastCardCode = codeStr;
    lastCardTime = millis();
    
    // Flash activity indicator
    flashRFIDIndicator(100);
    
    // Compare with master key (case insensitive)
    String master = String(MASTER_KEY);
    master.toUpperCase();
    if (codeStr.equalsIgnoreCase(master)) {
      // Immediately unlock regardless of network state
      Serial.println(F("[RFID] Master key detected"));
      unlockRelay("Master Key");
    } else if (!wifiConnected || !authenticated) {
      // Not connected or not authorised; deny access
      Serial.println(F("[RFID] Offline: denying access"));
      showTempMessage("Offline", "Access Denied", COLOR_MSG_ERR);
      flashRFIDIndicator(200);
    } else {
      // Send scan to the server
      sendRFIDScan(codeStr);
    }
  }
}

// ---------------------------------------------------------------------------
// Timer and presence handling
// ---------------------------------------------------------------------------

// Update relay timers and session runtime counters.  Called each loop
// iteration.  Updates the display once per second when a timer is
// active.
void updateTimers() {
  unsigned long now = millis();
  
  // Turn off RFID indicator after the duration expires
  if (rfidIndicatorEndTime != 0 && now > rfidIndicatorEndTime) {
    digitalWrite(PIN_LED_RFID, LOW);
    digitalWrite(PIN_RFID_LED, LOW);
    digitalWrite(PIN_RFID_BEEP, LOW);
    rfidIndicatorEndTime = 0;
  }
  
  // Door mode: countdown until relay end
  if (relayActive && strcmp(DEVICE_TYPE, "door") == 0) {
    if (now >= relayEndTime) {
      lockRelay();
      showMessage("Ready", "Scan card", COLOR_MSG_OK);
    } else {
      static unsigned long lastUpdate = 0;
      if (now - lastUpdate >= 1000) {
        // Display countdown seconds remaining
        uint32_t remaining = (relayEndTime - now + 999) / 1000;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s in %lu s", activeUser.c_str(), (unsigned long)remaining);
        showTempMessage("Access Granted", String(buf), COLOR_MSG_OK);
        lastUpdate = now;
      }
    }
  }
  
  // Machine mode: show elapsed session time
  if (relayActive && strcmp(DEVICE_TYPE, "machine") == 0) {
    static unsigned long lastUpdate = 0;
    if (now - lastUpdate >= 1000) {
      uint32_t seconds = (now - sessionStartTime) / 1000;
      uint32_t mins    = seconds / 60;
      uint32_t hours   = mins / 60;
      seconds %= 60;
      mins    %= 60;
      char timeBuf[16];
      snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu:%02lu", 
               (unsigned long)hours, (unsigned long)mins, (unsigned long)seconds);
      showRuntimeDisplay(activeUser, String(timeBuf), runtimeDisplayReset);
      runtimeDisplayReset = false;
      lastUpdate = now;
    }
  }
}

// Monitor card presence for require_card_present devices.  If the
// last scanned card has not been seen recently, end the session.
void checkCardPresence() {
  if (requireCardPresent && relayActive && strcmp(DEVICE_TYPE, "machine") == 0) {
    // If there is an active session and card not seen recently
    if ((millis() - lastCardTime) > CARD_PRESENT_TIMEOUT_MS) {
      // send session_end to server only if we have a session ID
      Serial.println(F("[RFID] Card removed, ending session"));
      if (wsConnected && authenticated && currentSessionId.length() > 0) {
        sendSessionEnd(currentSessionId);
        Serial.println(F("[SESSION] Sent end due to card removal"));
      }
      endSession(activeUser);
      lastCardCode = "";
    }
  }
}
