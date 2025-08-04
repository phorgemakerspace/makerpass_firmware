/*
 * MakerPass Firmware
 * Hardware Functions - RFID, relay, and pin management
 */

#include "makerpass.h"

void initializeHardware() {
  Serial.println("Initializing hardware...");
  
  // Initialize pins
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_RELAY, OUTPUT);
  pinMode(LED_RFID, OUTPUT);
  pinMode(RFID_LED, OUTPUT);
  pinMode(RFID_BEEP, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  // Set all outputs off initially
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED_RELAY, LOW);
  digitalWrite(LED_RFID, LOW);
  digitalWrite(RFID_LED, LOW);
  digitalWrite(RFID_BEEP, LOW);
  digitalWrite(RELAY_PIN, LOW);
  
  // Boot confirmation - LED test
  Serial.println("LED test...");
  for(int i = 0; i < 2; i++) {
    digitalWrite(LED_WIFI, HIGH);
    digitalWrite(LED_RELAY, HIGH);
    digitalWrite(LED_RFID, HIGH);
    digitalWrite(RFID_LED, HIGH);
    delay(100);
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_RELAY, LOW);
    digitalWrite(LED_RFID, LOW);
    digitalWrite(RFID_LED, LOW);
    delay(100);
  }
}

void setupRFID() {
  wiegand.begin(RFID_D0, RFID_D1);
  Serial.println("RFID reader initialized successfully");
  
  // Test RFID LED and beeper
  digitalWrite(RFID_LED, HIGH);
  digitalWrite(RFID_BEEP, HIGH);
  delay(200);
  digitalWrite(RFID_LED, LOW);
  digitalWrite(RFID_BEEP, LOW);
  Serial.println("RFID hardware test complete");
}

void controlRelay(bool state) {
  relayActive = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  digitalWrite(LED_RELAY, state ? HIGH : LOW);
  
  if (state) {
    relayStartTime = millis();
    Serial.print("Relay ACTIVATED - ");
    Serial.print(isDoor ? "Door" : "Machine");
    Serial.println(" should be energized");
  } else {
    Serial.print("Relay DEACTIVATED - ");
    Serial.print(isDoor ? "Door" : "Machine");
    Serial.println(" should be de-energized");

    // Reset activeUser to ensure proper display updates
    activeUser = "";
  }

  Serial.print("Physical relay pin ");
  Serial.print(RELAY_PIN);
  Serial.print(" set to: ");
  Serial.println(state ? "HIGH" : "LOW");
}
