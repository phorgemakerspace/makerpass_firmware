/*
 * MakerPass Firmware
 * 
 * This firmware is designed for ESP32-based MakerPass hardware to control access
 * to machines and doors using an RFID reader, interfacing with the MakerPass
 * plugin for Moodle.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "config.h"

// For the Wiegand library
#include "Wiegand.h"

// Display Pin Definitions (defined in platformio.ini build_flags for TFT_eSPI)
// TFT_eSPI will use: MOSI=21, SCLK=22, CS=-1, DC=14, RST=13

// Status LED Pin Definitions
#define LED_WIFI        27  // WiFi status LED
#define LED_RELAY       26  // Relay status LED
#define LED_RFID        25  // RFID status LED

// RFID hardware pins
#define RFID_D0         4   // Wiegand D0
#define RFID_D1         5   // Wiegand D1
#define RFID_LED        18  // RFID module LED
#define RFID_BEEP       19  // RFID module beeper

// Other pins
#define RELAY_PIN       23  // Main relay control

// Create objects
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
const unsigned long MAIN_SCREEN_TIMEOUT = 3000;  // Default message display time before returning to main screen
const unsigned long DEFAULT_DOOR_OPEN_TIME = 5000;  // Default door open time in milliseconds (5 seconds)

// Function declarations
bool connectToWifi();
bool retrieveResourceConfig();
void setupDisplay();
void setupRFID();
void handleRFID(uint32_t cardId);
bool checkAccess(String rfidCode);
void updateDisplay();
void showMainScreen();
void controlRelay(bool state);
void showMessage(String line1, String line2, String line3, uint16_t color, int duration);

void setup() {
  // Set boot pins to safe states
  pinMode(0, INPUT_PULLUP);  // GPIO0 - boot pin
  pinMode(2, INPUT_PULLUP);  // GPIO2 - boot pin
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("");
  Serial.println("=== MAKERPASS SYSTEM STARTUP ===");
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
  
  // Store configuration values from config.h
  apiUrl = String(API_URL);
  apiKey = String(API_KEY);
  equipmentId = String(EQUIPMENT_ID);
  
  // Set device configuration from config.h
  isDoor = (String(DEVICE_TYPE).equalsIgnoreCase("door"));
  doorOpenTime = DEFAULT_DOOR_OPEN_TIME;  // Use internal default
  cardPresentRequired = false;  // Default value, may be overridden by API
  
  // Log configuration
  Serial.print("Device type: ");
  Serial.println(isDoor ? "Door" : "Machine");
  
  // Setup display
  Serial.println("Initializing display...");
  setupDisplay();
  
  // Show startup message
  showMessage("MAKERPASS", "STARTING", "Please wait...", TFT_BLUE, 0);
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  showMessage("WIFI", "CONNECTING", "Please wait...", TFT_YELLOW, 0);
  isWifiConnected = connectToWifi();
  
  if (isWifiConnected) {
    showMessage("WIFI", "CONNECTED", "Getting config...", TFT_GREEN, 0);
    
    // Get resource configuration from API
    Serial.println("Retrieving resource configuration...");
    bool configSuccess = retrieveResourceConfig();
    if (configSuccess) {
      showMessage("CONFIG", "LOADED", resourceName, TFT_GREEN, 0);
      systemReady = true;
    } else {
      showMessage("CONFIG", "ERROR", "Master key only", TFT_RED, 0);  // Stay on error screen
      Serial.println("FATAL: Cannot retrieve configuration from API");
      Serial.println("System will operate with master key only");
      // Don't exit - allow master key access
      systemReady = false;
    }
  } else {
    showMessage("WIFI", "ERROR", "Master key only", TFT_RED, 0);  // Stay on error screen
    Serial.println("FATAL: Cannot connect to WiFi");
    Serial.println("System will operate with master key only");
    // Don't exit - allow master key access
    systemReady = false;
  }
  
  // Configure time
  configTime(0, 0, "pool.ntp.org");
  
  // Setup RFID reader (always setup for master key access)
  Serial.println("Initializing RFID reader...");
  setupRFID();
  
  // Final startup message
  if (systemReady) {
    showMessage("MAKERPASS", "READY", "Scan RFID card", TFT_GREEN, MAIN_SCREEN_TIMEOUT);
    Serial.println("=== SETUP COMPLETE ===");
    // Show main screen - this will be called by showMessage automatically
  } else {
    // System not fully configured, but master key will work
    Serial.println("=== SETUP COMPLETE (MASTER KEY MODE) ===");
    showMainScreen();
  }
}

void showMessage(String line1, String line2, String line3, uint16_t color, int duration) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(color);
  tft.setTextFont(4);  // Increased from 2
  tft.setTextSize(1);
  
  // Center text on 320x170 display (landscape)
  int x = 20;  // Adjusted for larger text
  
  tft.setCursor(x, 30);
  tft.print(line1);
  
  tft.setCursor(x, 70);
  tft.print(line2);
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);  // Increased from 1
  tft.setTextSize(1);
  tft.setCursor(x, 120);
  tft.print(line3);
  
  delay(duration);
  
  // Only return to main screen if duration > 0 (permanent messages stay on screen)
  if (duration > 0) {
    showMainScreen();
  }
}

void loop() {
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
  if (wiegand.available()) {
    digitalWrite(LED_RFID, HIGH);
    uint32_t cardId = wiegand.getCode();
    
    // Update the last card time
    lastCardTime = millis();
    
    // Convert card ID to string
    char cardIdStr[16];
    sprintf(cardIdStr, "%08X", cardId);
    
    handleRFID(cardId);
    digitalWrite(LED_RFID, LOW);
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
        // No need to show main screen here - the timer below will handle it
      }
    }
    
    // Update runtime display approximately once per second (only for machines)
    static unsigned long lastDisplayUpdate = 0;
    if (relayActive && !isDoor && millis() - lastDisplayUpdate >= 1000) {
      updateDisplay();
      lastDisplayUpdate = millis();
    }
  }
  
  delay(10); // Small delay to prevent CPU overload
}

// === MAKERPASS SYSTEM FUNCTIONS ===

bool connectToWifi() {
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  
  // Disconnect and clear any previous connections
  WiFi.disconnect(true);
  delay(1000);
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Begin connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Print status every 10 attempts
    if (attempts % 10 == 0) {
      Serial.println();
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
    }
    
    // Timeout after 30 seconds
    if (millis() - startTime > 30000) {
      Serial.println("\nWiFi connection timeout");
      Serial.print("Final status: ");
      Serial.println(WiFi.status());
      return false;
    }
  }
  
  Serial.println("\nWiFi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  return true;
}

bool retrieveResourceConfig() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - cannot retrieve config");
    return false;
  }
  
  HTTPClient http;
  String url = apiUrl + "/api/makerpass.php/resource";
  
  Serial.print("API URL: ");
  Serial.println(url);
  
  http.begin(url);
  http.addHeader("X-API-Key", apiKey);
  http.addHeader("X-Resource-ID", equipmentId);
  
  int httpCode = http.GET();
  bool success = false;
  
  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.print("API Response: ");
    Serial.println(response);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      // Get device name and settings from API
      resourceName = doc["name"].as<String>();
      
      // Get card_present_required from API (defaults to false)
      if (doc["card_present_required"].is<bool>()) {
        cardPresentRequired = doc["card_present_required"].as<bool>();
      } else {
        cardPresentRequired = false;
      }
      
      Serial.println("Resource configuration retrieved successfully");
      Serial.print("Resource name: ");
      Serial.println(resourceName);
      Serial.print("Resource type (from config): ");
      Serial.println(isDoor ? "Door" : "Machine");
      Serial.print("Card present required: ");
      Serial.println(cardPresentRequired ? "YES" : "NO");
      
      success = true;
    } else {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
      // Keep config.h values, just reset card present requirement
      cardPresentRequired = false;
    }
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
    
    // Handle specific error responses
    if (httpCode == 400) {
      Serial.println("Bad request - check API parameters");
    } else if (httpCode == 401) {
      Serial.println("Unauthorized - check API key");
    } else if (httpCode == 405) {
      Serial.println("Method not allowed - check endpoint");
    } else if (httpCode == 500) {
      Serial.println("Server error");
    }
    
    // Try to parse error response
    if (httpCode > 0) {
      String errorResponse = http.getString();
      if (errorResponse.length() > 0) {
        Serial.print("Error response: ");
        Serial.println(errorResponse);
        
        JsonDocument errorDoc;
        if (deserializeJson(errorDoc, errorResponse) == DeserializationError::Ok) {
          if (errorDoc["error"].is<String>()) {
            Serial.print("Server error message: ");
            Serial.println(errorDoc["error"].as<String>());
          }
        }
      }
    }
    
    // Keep config.h values, just reset card present requirement
    cardPresentRequired = false;
  }
  
  http.end();
  return success;
}

void setupDisplay() {
  // Manual reset sequence
  pinMode(13, OUTPUT);  // Reset pin
  pinMode(14, OUTPUT);  // DC pin
  
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  
  // Initialize
  tft.init();
  tft.setRotation(3);  // Landscape Inverted (320x170)
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("Display initialized successfully");
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
  // Serial.print("WiFi Connected: ");
  // Serial.println(isWifiConnected ? "YES" : "NO");
  
  // Visual/audio feedback
  digitalWrite(RFID_BEEP, HIGH);
  digitalWrite(RFID_LED, HIGH);
  showMessage("SCANNING", "CARD", "Please wait...", TFT_YELLOW, 0);  // Stay on screen during check
  
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
      showMessage("ACCESS", "DENIED", "Use master key", TFT_RED, MAIN_SCREEN_TIMEOUT);
      Serial.println("========================================");
      return;
    }
    
    // Set default values for master key mode when system not ready
    if (!systemReady) {
      // Don't set resourceName here - display logic handles it based on systemReady state
      // Device type and relay time are already set from config.h
      cardPresentRequired = false;
    }
    
    // Normal system operation for both API users and master key
    // Different behavior based on device type and current state
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
      activeUser = "";
      showMessage("ACCESS", "ENDED", "Runtime: " + runtimeStr, TFT_BLUE, MAIN_SCREEN_TIMEOUT);
    } else {
      // For doors or inactive machines, turn on the relay
      Serial.print("Activating ");
      Serial.println(isDoor ? "door" : "machine");
      controlRelay(true);
      activeUser = String(cardIdStr);
      if (isDoor) {
        showMessage("ACCESS", "GRANTED", "Door unlocked", TFT_GREEN, MAIN_SCREEN_TIMEOUT);
      } else {
        // For machines, show brief confirmation then switch to runtime display
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN);
        tft.setTextFont(4);
        tft.setTextSize(1);
        
        tft.setCursor(20, 30);
        tft.print("ACCESS");
        tft.setCursor(20, 70);
        tft.print("GRANTED");
        
        tft.setTextColor(TFT_WHITE);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setCursor(20, 120);
        tft.print("Machine active");
        
        delay(1500);
        
        // Switch directly to runtime display (stay there)
        updateDisplay();
      }
    }
  } else {
    Serial.println("ACCESS DENIED");
    
    // Check if it was master key attempt or system/API failure
    if (String(cardIdStr).equals(String(MASTER_RFID_KEY))) {
      Serial.println("Reason: Master key failed (check MASTER_RFID_KEY setting)");
      showMessage("ACCESS", "DENIED", "Master key error", TFT_RED, MAIN_SCREEN_TIMEOUT);
    } else if (!systemReady) {
      Serial.println("Reason: System offline - use master key");
      showMessage("ACCESS", "DENIED", "Use master key", TFT_RED, MAIN_SCREEN_TIMEOUT);
    } else if (!isWifiConnected) {
      Serial.println("Reason: No WiFi connection");
      showMessage("ACCESS", "DENIED", "No WiFi", TFT_RED, MAIN_SCREEN_TIMEOUT);
    } else {
      Serial.println("Reason: Unknown user or API error");
      // Show the card ID that the database would see - keep this one longer for readability
      showMessage("UNKNOWN USER", cardIdStr, "Access denied", TFT_RED, MAIN_SCREEN_TIMEOUT + 1000);
    }
  }
  
  Serial.print("Current relay state: ");
  Serial.println(relayActive ? "ACTIVE" : "INACTIVE");
  Serial.print("Active user: ");
  Serial.println(activeUser.length() > 0 ? activeUser : "None");
  Serial.println("========================================");
}

bool checkAccess(String rfidCode) {
  Serial.print("Checking card: ");
  Serial.println(rfidCode);
  Serial.print("DATABASE WILL SEE: RFID='");
  Serial.print(rfidCode);
  Serial.print("', RESOURCE_ID='");
  Serial.print(equipmentId);
  Serial.println("'");
  
  // Check master key first (works offline and online)
  if (rfidCode.equals(String(MASTER_RFID_KEY))) {
    Serial.println("✓ MASTER KEY MATCH - Access granted");
    return true;
  }
  
  Serial.print("Master key (");
  Serial.print(MASTER_RFID_KEY);
  Serial.println(") does not match");
  
  // If system is not ready, only master key works
  if (!systemReady) {
    Serial.println("✗ System offline - only master key allowed");
    return false;
  }
  
  // System is ready, check API for non-master keys
  Serial.println("System ready - checking API...");
  
  if (!isWifiConnected) {
    Serial.println("✗ No WiFi connection - access denied");
    return false;
  }
  
  HTTPClient http;
  String url = apiUrl + "/api/makerpass.php/check_access";
  
  Serial.print("API Request URL: ");
  Serial.println(url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", apiKey);
  http.addHeader("X-RFID", rfidCode);
  http.addHeader("X-Resource-ID", equipmentId);
  
  Serial.print("API Request headers: ");
  Serial.println("X-API-Key: " + apiKey);
  Serial.println("X-RFID: " + rfidCode);
  Serial.println("X-Resource-ID: " + equipmentId);
  
  int httpCode = http.POST("");  // Empty body since data is in headers
  bool access = false;
  
  Serial.print("API Response code: ");
  Serial.println(httpCode);
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.print("API Response body: ");
    Serial.println(response);
    Serial.println("^ This is exactly what the database returned");
    
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error) {
      String status = responseDoc["status"].as<String>();
      access = (status == "granted");
      
      Serial.print("DATABASE DECISION: ");
      status.toUpperCase();  // Modify the string in place
      Serial.println(status);
      
      if (!access && responseDoc["reason"].is<String>()) {
        String reason = responseDoc["reason"].as<String>();
        Serial.print("✗ Access denied reason: ");
        Serial.println(reason);
        
        // Log specific denial reasons
        if (reason == "invalid_rfid") {
          Serial.println("RFID card not found in database");
        } else if (reason == "access_not_permitted") {
          Serial.println("User does not have permission for this resource");
        }
      } else if (access) {
        Serial.println("✓ API Access granted");
      }
    } else {
      Serial.print("✗ JSON parsing error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("✗ HTTP error: ");
    Serial.println(httpCode);
    
    // Handle specific error responses
    if (httpCode == 400) {
      Serial.println("Bad request - check RFID format or resource ID");
    } else if (httpCode == 401) {
      Serial.println("Unauthorized - check API key");
    } else if (httpCode == 405) {
      Serial.println("Method not allowed - check endpoint");
    } else if (httpCode == 500) {
      Serial.println("Server error");
    }
    
    // Try to parse error response
    if (httpCode > 0) {
      String errorResponse = http.getString();
      if (errorResponse.length() > 0) {
        Serial.print("Error response: ");
        Serial.println(errorResponse);
        Serial.println("^ This is what the database/server returned");
        
        JsonDocument errorDoc;
        if (deserializeJson(errorDoc, errorResponse) == DeserializationError::Ok) {
          if (errorDoc["error"].is<String>()) {
            Serial.print("Server error message: ");
            Serial.println(errorDoc["error"].as<String>());
          }
        }
      }
    }
  }
  
  http.end();
  return access;
}

void updateDisplay() {
  static bool lastRelayState = false;
  static String lastActiveUser = "";
  
  // Only do full screen refresh if relay state changed or user changed
  bool needFullRefresh = (relayActive != lastRelayState) || (activeUser != lastActiveUser);
  
  if (needFullRefresh) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(4);  // Increased from 2
    tft.setTextSize(1);
    
    // Display resource name at top
    tft.setCursor(10, 10);
    if (resourceName.length() > 0) {
      tft.print(resourceName);
    } else if (systemReady) {
      tft.print("MakerPass Device");
    } else {
      tft.print("Master Key Mode");
    }
    
    // Display status
    tft.setCursor(10, 45);
    if (relayActive) {
      tft.setTextColor(TFT_GREEN);
      tft.print("ACTIVE");
      
      // Show active user
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(2);  // Increased from 1
      tft.setTextSize(1);
      tft.setCursor(10, 75);
      tft.print("User: ");
      tft.print(activeUser);
      
    } else {
      tft.print("Status: ");
      tft.setTextColor(TFT_RED);
      tft.print("INACTIVE");
      
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(2);  // Increased from 1
      tft.setTextSize(1);
      tft.setCursor(10, 80);
      if (systemReady) {
        tft.print("Scan RFID card to activate");
      } else {
        tft.print("Scan master key to activate");
      }
    }
    
    // WiFi status at bottom
    tft.setCursor(10, 145);
    tft.setTextFont(2);  // Increased from 1
    tft.print("WiFi: ");
    if (isWifiConnected) {
      tft.setTextColor(TFT_GREEN);
      tft.print("Connected");
    } else {
      tft.setTextColor(TFT_RED);
      tft.print("Disconnected");
    }
    
    lastRelayState = relayActive;
    lastActiveUser = activeUser;
  }
  
  // Update runtime area without clearing screen (only when relay is active)
  if (relayActive) {
    // Clear just the runtime area by drawing a black rectangle
    tft.fillRect(10, 100, 300, 40, TFT_BLACK);  // Adjusted height for larger text
    
    // Show runtime
    unsigned long runTime = (millis() - relayStartTime) / 1000;
    unsigned long hours = runTime / 3600;
    unsigned long minutes = (runTime % 3600) / 60;
    unsigned long seconds = runTime % 60;
    
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2);  // Increased from 1
    tft.setTextSize(1);
    tft.setCursor(10, 105);
    tft.print("Runtime: ");
    if (hours < 10) tft.print("0");
    tft.print(hours);
    tft.print(":");
    if (minutes < 10) tft.print("0");
    tft.print(minutes);
    tft.print(":");
    if (seconds < 10) tft.print("0");
    tft.print(seconds);
    
    // For doors, show remaining time (only if system is ready and configured)
    if (isDoor && systemReady) {
      unsigned long remainingTime = (doorOpenTime - (millis() - relayStartTime)) / 1000;
      if (remainingTime > 0) {
        tft.setCursor(10, 125);
        tft.print("Door closes in: ");
        tft.print(remainingTime);
        tft.print("s");
      }
    }
  }
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
  }
  
  Serial.print("Physical relay pin ");
  Serial.print(RELAY_PIN);
  Serial.print(" set to: ");
  Serial.println(state ? "HIGH" : "LOW");
}

void showMainScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(4);  // Increased from 2
  tft.setTextSize(1);
  
  // Display device name at top
  tft.setCursor(10, 15);
  if (resourceName.length() > 0) {
    tft.print(resourceName);
  } else if (systemReady) {
    tft.print("MakerPass Device");
  } else {
    tft.print("Master Key Mode");
  }
  
  // Display main message with different text/color based on system state
  tft.setCursor(10, 65);
  tft.setTextFont(4);  // Keep large for main message
  
  if (!systemReady) {
    tft.setTextColor(TFT_BLUE);
    tft.print("System Offline.");
    tft.setCursor(10, 100);
    tft.print("Scan Master Key.");
  } else if (!isWifiConnected) {
    tft.setTextColor(TFT_BLUE);
    tft.print("WiFi Unavailable.");
    tft.setCursor(10, 100);
    tft.print("Scan Master Key.");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.print("Ready. Scan card.");
  }
  
  // WiFi status at bottom
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);  // Increased from 1
  tft.setTextSize(1);
  tft.setCursor(10, 130);
  tft.print("WiFi: ");
  if (isWifiConnected) {
    tft.setTextColor(TFT_GREEN);
    tft.print("Connected");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("Disconnected");
  }
}

