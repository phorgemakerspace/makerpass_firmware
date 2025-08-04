/*
 * MakerPass Firmware
 * Access Control - RFID handling and access management
 */

#include "makerpass.h"

void handleRFID(uint32_t cardId) {
  // Convert card ID to string for consistent logging
  char cardIdStr[16];
  sprintf(cardIdStr, "%08X", cardId);

  Serial.println("========================================");
  Serial.println("RFID CARD SCAN EVENT");
  Serial.print("Card ID: ");
  Serial.println(cardIdStr);
  Serial.print("System Ready: ");
  Serial.println(systemReady ? "YES" : "NO (Master key only)");

  // Visual/audio feedback
  digitalWrite(RFID_BEEP, HIGH);
  digitalWrite(RFID_LED, HIGH);
  showMessage("Verifying", "Credentials", "Please wait...", TFT_YELLOW, 0);

  // Check access with API (or master key)
  Serial.println("Checking access...");
  bool access = checkAccess(String(cardIdStr));

  // Turn off feedback after check completes
  digitalWrite(RFID_BEEP, LOW);
  digitalWrite(RFID_LED, LOW);

  // Ensure scanning message is visible for at least a brief moment
  delay(500);

  if (access) {
    Serial.println("ACCESS GRANTED");
    Serial.print("Access method: ");
    Serial.println(String(cardIdStr).equals(String(MASTER_RFID_KEY)) ? "Master key" : "API validation");

    // If system is not ready, only master key works
    if (!systemReady && !String(cardIdStr).equals(String(MASTER_RFID_KEY))) {
      Serial.println("System offline - only master key allowed");
      showMessage("ACCESS", "DENIED", "Use master key", TFT_RED, MESSAGE_TIMEOUT);
      Serial.println("========================================");
      return;
    }

    // Set default values for master key mode when system not ready
    if (!systemReady) {
      cardPresentRequired = false;
    }

    // Normal system operation for both API users and master key
    if (!isDoor && relayActive) {
      // For machines that are active, scanning turns them off (toggle behavior)
      Serial.println("Machine toggle: Turning OFF");

      // Calculate total runtime for display
      unsigned long totalRunTime = (millis() - relayStartTime) / 1000;
      unsigned long hours = totalRunTime / 3600;
      unsigned long minutes = (totalRunTime % 3600) / 60;
      unsigned long seconds = totalRunTime % 60;

      // Format runtime string
      String runtimeStr = "";
      if (hours < 10) runtimeStr += "0";
      runtimeStr += String(hours) + ":";
      if (minutes < 10) runtimeStr += "0";
      runtimeStr += String(minutes) + ":";
      if (seconds < 10) runtimeStr += "0";
      runtimeStr += String(seconds);

      controlRelay(false);
      showMessage("Session", "Ended", "Runtime: " + runtimeStr, TFT_BLUE, MESSAGE_TIMEOUT);
      // Important: Set activeUser to empty AFTER showing the message to prevent display override
      activeUser = "";
    } else {
      // For doors or inactive machines, turn on the relay
      Serial.print("Activating ");
      Serial.println(isDoor ? "door" : "machine");
      controlRelay(true);
      activeUser = String(cardIdStr);

      // Directly transition to runtime screen or door timer countdown
      updateDisplay();
    }
  } else {
    Serial.println("ACCESS DENIED");

    // Check if it was master key attempt or system/API failure
    if (String(cardIdStr).equals(String(MASTER_RFID_KEY))) {
      Serial.println("Reason: Master key failed (check MASTER_RFID_KEY setting)");
      showMessage("ACCESS", "DENIED", "Master key error", TFT_RED, MESSAGE_TIMEOUT);
    } else if (!systemReady) {
      Serial.println("Reason: System offline - use master key");
      showMessage("ACCESS", "DENIED", "Use master key", TFT_RED, MESSAGE_TIMEOUT);
    } else if (!isWifiConnected) {
      Serial.println("Reason: No WiFi connection");
      showMessage("ACCESS", "DENIED", "No WiFi", TFT_RED, MESSAGE_TIMEOUT);
    } else {
      Serial.println("Reason: Unknown user or API error");
      showMessage("UNKNOWN USER", cardIdStr, "Access denied", TFT_RED, MESSAGE_TIMEOUT + 1000);
    }
  }

  Serial.print("Current relay state: ");
  Serial.println(relayActive ? "Active" : "Inactive");
  Serial.print("Active user: ");
  Serial.println(activeUser.length() > 0 ? activeUser : "None");
  Serial.println("========================================");
}
