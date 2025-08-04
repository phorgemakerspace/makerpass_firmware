/*
 * MakerPass Firmware
 * Network Functions - WiFi and API communication
 */

#include "makerpass.h"

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
    
    cardPresentRequired = false;
  }
  
  http.end();
  return success;
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
  
  int httpCode = http.POST("");
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
      status.toUpperCase();
      Serial.println(status);
      
      if (!access && responseDoc["reason"].is<String>()) {
        String reason = responseDoc["reason"].as<String>();
        Serial.print("✗ Access denied reason: ");
        Serial.println(reason);
        
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
    
    if (httpCode == 400) {
      Serial.println("Bad request - check RFID format or resource ID");
    } else if (httpCode == 401) {
      Serial.println("Unauthorized - check API key");
    } else if (httpCode == 405) {
      Serial.println("Method not allowed - check endpoint");
    } else if (httpCode == 500) {
      Serial.println("Server error");
    }
    
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
