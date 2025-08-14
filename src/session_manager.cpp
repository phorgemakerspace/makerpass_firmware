// Session management functions for MakerPass firmware
// This module handles relay control and session management

#include "session_manager.h"
#include "config.h"
#include "pins.h"
#include "constants.h"
#include "ui_manager.h"
#include "websocket_manager.h"

extern bool relayActive;
extern unsigned long relayEndTime;
extern String activeUser;
extern String currentSessionId;
extern unsigned long sessionStartTime;
extern bool runtimeDisplayReset;

// Briefly illuminate the RFID activity LED and reader LED/beeper.  The
// actual turn‑off happens asynchronously in updateTimers().
void flashRFIDIndicator(uint16_t durationMs) {
  extern unsigned long rfidIndicatorEndTime;
  Serial.print(F("[RFID] Flash indicator for "));
  Serial.print(durationMs);
  Serial.println(F(" ms"));
  digitalWrite(PIN_LED_RFID, HIGH);
  digitalWrite(PIN_RFID_LED, HIGH);
  digitalWrite(PIN_RFID_BEEP, HIGH);
  rfidIndicatorEndTime = millis() + durationMs;
}

// Energise the relay for a door and display a countdown.  The relay
// remains energised for RELAY_DOOR_DURATION_MS and then turns off.
void unlockRelay(const String &userName) {
  relayActive = true;
  relayEndTime = millis() + RELAY_DOOR_DURATION_MS;
  digitalWrite(PIN_RELAY, HIGH);
  digitalWrite(PIN_LED_RELAY, HIGH);
  activeUser = userName;
  // Initial UI: Access Granted with starting seconds
  uint32_t remaining = (relayEndTime - millis() + 999) / 1000;
  showDoorCountdown("Access Granted", String(remaining) + " s", true);
  Serial.print(F("[RELAY] Door unlocked for user: "));
  Serial.println(userName);
}

// De‑energise the relay and clear related state
void lockRelay() {
  relayActive = false;
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED_RELAY, LOW);
  activeUser = "";
  relayEndTime = 0;
}

// Start a machine session.  The relay is energised until the
// session ends.  The sessionId may be empty if the server did not
// provide one (e.g. access_granted in machine mode).
void startSession(const String &sessionId, const String &userName) {
  currentSessionId = sessionId;
  activeUser       = userName;
  sessionStartTime = millis();
  runtimeDisplayReset = true;  // Reset runtime display for new session
  relayActive      = true;
  digitalWrite(PIN_RELAY, HIGH);
  digitalWrite(PIN_LED_RELAY, HIGH);
  // Display user and initial elapsed time
  showMessage(userName, "Session Started", COLOR_MSG_OK);
  Serial.print(F("[SESSION] Started for user: "));
  Serial.println(userName);
}

// End a machine session.  Turn off the relay and clear session
// variables.  Display that the session has ended.
void endSession(const String &userName) {
  lockRelay();
  currentSessionId = "";
  showTempMessage("Session Ended", userName, COLOR_MSG_WARN);
  Serial.print(F("[SESSION] Ended for user: "));
  Serial.println(userName);
}
