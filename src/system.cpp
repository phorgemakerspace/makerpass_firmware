/*
 * MakerPass Firmware
 * System Functions - Initialization and main loop
 */

#include "makerpass.h"

void initializeSystem() {
  // Store configuration values from config.h
  apiUrl = String(API_URL);
  apiKey = String(API_KEY);
  equipmentId = String(EQUIPMENT_ID);
  
  // Set device configuration from config.cpp
  isDoor = (String(DEVICE_TYPE).equalsIgnoreCase("door"));
  doorOpenTime = DOOR_CLOSE_TIME;  // Use configurable value
  cardPresentRequired = false;
  
  // Log configuration
  Serial.print("Device type: ");
  Serial.println(isDoor ? "Door" : "Machine");
  
  // Setup display
  Serial.println("Initializing display...");
  setupDisplay();
  
  // Show startup message
  showMessage("MakerPass", "Starting", "Please wait...", TFT_BLUE, 0);
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  showMessage("WiFi", "Connecting", "Please wait...", TFT_YELLOW, 0);
  isWifiConnected = connectToWifi();
  
  if (isWifiConnected) {
    showMessage("WiFi", "Connected", "Getting configuration...", TFT_GREEN, 0);

    // Get resource configuration from API
    Serial.println("Retrieving resource configuration...");
    bool configSuccess = retrieveResourceConfig();
    if (configSuccess) {
      showMessage("Configuration", "Loaded", resourceName, TFT_GREEN, 0);
      systemReady = true;
    } else {
      showMessage("Configuration", "Error", "Master key only", TFT_RED, 0);
      Serial.println("FATAL: Cannot retrieve configuration from API");
      Serial.println("System will operate with master key only");
      systemReady = false;
    }
  } else {
    showMessage("WiFi", "Error", "Master key only", TFT_RED, 0);
    Serial.println("FATAL: Cannot connect to WiFi");
    Serial.println("System will operate with master key only");
    systemReady = false;
  }
  
  // Configure time
  configTime(0, 0, "pool.ntp.org");
  
  // Setup RFID reader (always setup for master key access)
  Serial.println("Initializing RFID reader...");
  setupRFID();
  
  // Final startup message
  if (systemReady) {
    showMessage("MakerPass", "Ready", "Initializing...", TFT_GREEN, 1000);
    Serial.println("=== SETUP COMPLETE ===");
  } else {
    Serial.println("=== SETUP COMPLETE (MASTER KEY MODE) ===");
    showMainScreen();
  }
}

void runMainLoop() {
  // Check WiFi connection and reconnect if necessary (only if system was ready)
  if (systemReady) {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED_WIFI, LOW);
      isWifiConnected = connectToWifi();
    } else {
      digitalWrite(LED_WIFI, HIGH);
      isWifiConnected = true;
    }
  }

  // Check for new card reads (always check for master key)
  // RFID card processing - clean and simple
  if (wiegand.available()) {
    uint32_t cardId = wiegand.getCode();
    
    // Log successful detection
    Serial.print("RFID card detected: ");
    Serial.println(cardId, HEX);
    
    // Process card read
    digitalWrite(LED_RFID, HIGH);
    lastCardTime = millis();

    // Convert card ID to string
    char cardIdStr[16];
    sprintf(cardIdStr, "%08X", cardId);
    
    Serial.print("Processing card: ");
    Serial.println(cardIdStr);

    handleRFID(cardId);
    digitalWrite(LED_RFID, LOW);
  }

  // Check for message timeout
  if (messageActive && millis() >= messageExpireTime) {
    messageActive = false;
    messageExpireTime = 0;
    if (relayActive) {
      updateDisplay(); // Show runtime screen
    } else {
      showMainScreen(); // Show main screen
    }
  }

  // Only do full system operations if system is ready OR if using master key
  if (systemReady || (activeUser.length() > 0 && activeUser.equals(String(MASTER_RFID_KEY)))) {
    // Check if card present is required and card has been removed
    if (relayActive && cardPresentRequired) {
      // Consider card removed if no read in the last 2 seconds
      if (millis() - lastCardTime > 2000) {
        Serial.println("Card removed - deactivating relay");
        controlRelay(false);
        activeUser = "";
        showMainScreen();
      }
    }

    // For doors, automatically deactivate relay after the configured time
    if (relayActive && isDoor) {
      if (millis() - relayStartTime >= doorOpenTime) {
        Serial.println("Door timer expired - deactivating relay");
        controlRelay(false);
        activeUser = "";
      }
    }

    // Update display approximately once per second
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate >= 1000) {
      updateDisplay();
      lastDisplayUpdate = millis();
    }
  }

  delay(10); // Small delay to prevent CPU overload
}
