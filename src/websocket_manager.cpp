// WebSocket management functions for MakerPass firmware
// This module handles WebSocket connection and message processing

#include "websocket_manager.h"
#include "config.h"
#include "constants.h"
#include "ui_manager.h"
#include "session_manager.h"
#include <WiFiClientSecure.h>
#include <time.h>

extern WebSocketsClient webSocket;
extern bool wsConnected;
extern bool authenticated;
extern bool resourceEnabled;
extern bool requireCardPresent;
extern String resourceName;
extern unsigned long lastPongTime;

// Initialise the WebSocket client, specify the server and path and
// register the event callback.  A reconnect interval ensures that
// lost connections are re‑established automatically.
void initWebSocket() {
  // Synchronize time for SSL certificate validation
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("[SSL] Synchronizing time");
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 8 * 3600 * 2 && attempts < 15) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
  }
  Serial.println("");
  Serial.print("[SSL] Time synchronized: ");
  Serial.println(ctime(&now));
  
  // Use SSL connection with proper certificate handling
  webSocket.beginSSL(WS_HOST, WS_PORT, WS_PATH);
  
  webSocket.onEvent([](WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
      case WStype_DISCONNECTED:
        Serial.println(F("[WS] Disconnected"));
        wsConnected = false;
        authenticated = false;
        resourceEnabled = false;
        showMessage("Offline", "Master Key Only", COLOR_MSG_WARN);
        break;
      case WStype_CONNECTED: {
        Serial.print(F("[WS] Connected to: "));
        Serial.println((const char *)payload);
        wsConnected = true;
        // Initialize activity timing (server sends pings, we track last activity)
        extern unsigned long lastPongTime;
        lastPongTime = millis();
        // immediately send device_auth
        sendDeviceAuth();
        break;
      }
      case WStype_TEXT: {
        String msg = String((const char *)payload);
        Serial.print(F("[WS] Text: "));
        Serial.println(msg);
        handleIncomingMessage(msg);
        break;
      }
      case WStype_BIN:
        Serial.println(F("[WS] Binary message received, ignoring"));
        break;
      case WStype_PING:
        // reply with pong is handled automatically by the library
        break;
      case WStype_PONG:
        // update last pong time for keep‑alive monitoring
        Serial.println(F("[WS] Received WebSocket pong"));
        lastPongTime = millis();
        break;
      case WStype_ERROR:
        Serial.println(F("[WS] Error"));
        break;
      default:
        break;
    }
  });
  // Reconnect every 5 seconds if the connection drops
  webSocket.setReconnectInterval(5000);
}

// Send a device_auth message when the WebSocket is connected.  The
// server uses the resource_id and API key to authenticate the device
// before allowing any RFID events to be processed.
void sendDeviceAuth() {
  JsonDocument doc;
  doc["type"]        = "device_auth";
  doc["resource_id"] = RESOURCE_ID;
  doc["api_key"]     = API_KEY;
  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
}

// Dispatch a raw JSON message received over the WebSocket.  The
// payload is parsed into a JsonDocument and passed to
// processJsonMessage() for further handling.  If parsing fails
// the message is ignored.
void handleIncomingMessage(const String &message) {
  // Any message from server counts as activity
  extern unsigned long lastPongTime;
  lastPongTime = millis();
  
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err) {
    Serial.print(F("[JSON] Deserialization failed: "));
    Serial.println(err.c_str());
    return;
  }
  processJsonMessage(doc);
}

// Interpret and act upon a JSON message from the server.
void processJsonMessage(JsonDocument &doc) {
  const char* type = doc["type"] | "";
  if (strcmp(type, "auth_success") == 0) {
    authenticated      = true;
    resourceEnabled    = doc["enabled"] | false;
    requireCardPresent = doc["require_card_present"] | false;
    resourceName       = doc["resource_name"] | String(RESOURCE_ID);
    Serial.println(F("[AUTH] Success"));
    if (!resourceEnabled) {
      showTempMessage("Resource Disabled", "", COLOR_MSG_WARN);
    } else {
      showIdleScreen(); // Show the new idle screen layout
    }
  } else if (strcmp(type, "ping") == 0) {
    // Server sent us a ping, respond with pong
    Serial.println(F("[WS] Received ping from server, sending pong"));
    JsonDocument pongDoc;
    pongDoc["type"] = "pong";
    String json;
    serializeJson(pongDoc, json);
    webSocket.sendTXT(json);
    // Update our last activity time
    lastPongTime = millis();
  } else if (strcmp(type, "pong") == 0) {
    // Server responded to our ping (though we don't send them anymore)
    Serial.println(F("[WS] Received pong from server"));
    lastPongTime = millis();
  } else if (strcmp(type, "access_granted") == 0) {
    String userName = doc["user_name"] | doc["user"] | "User";
    if (strcmp(DEVICE_TYPE, "door") == 0) {
      unlockRelay(userName);
    } else {
      // For machines, start a session without id
      startSession("", userName);
    }
  } else if (strcmp(type, "access_denied") == 0) {
    String reason = doc["reason"] | doc["message"] | "Denied";
    Serial.print(F("[ACCESS] Denied: "));
    Serial.println(reason);
    showTempMessage("Access Denied", reason, COLOR_MSG_ERR);
    // brief flash of the RFID LED to indicate denial
    flashRFIDIndicator(200);
  } else if (strcmp(type, "session_started") == 0) {
    const char* sid = doc["session_id"] | "";
    String userName = doc["user_name"] | doc["user"] | "User";
    startSession(String(sid), userName);
  } else if (strcmp(type, "session_ended") == 0) {
    String userName = doc["user_name"] | doc["user"] | "";
    endSession(userName);
  } else if (strcmp(type, "error") == 0 || strcmp(type, "auth_error") == 0) {
    String errorMsg = doc["message"] | "Unknown error";
    Serial.print(F("[ERROR] "));
    Serial.println(errorMsg);
    authenticated = false;
    wsConnected   = false;
    showTempMessage("Error", errorMsg, COLOR_MSG_ERR);
  } else {
    Serial.print(F("[WARN] Unrecognised message type: "));
    Serial.println(type);
  }
}

// Handle WebSocket keep-alive - Server initiates pings, we just monitor timeout
void handleWebSocketKeepAlive() {
  extern bool wsConnected, authenticated;
  extern unsigned long lastPongTime;
  
  if (wsConnected && authenticated) {
    unsigned long now = millis();
    // Server sends pings every 5 minutes, we have 15-minute timeout
    // We only need to check if we haven't heard from server in too long
    if (now - lastPongTime > PONG_TIMEOUT_MS) {
      Serial.print(F("[WS] Server timeout, closing socket. Last activity was "));
      Serial.print((now - lastPongTime) / 1000);
      Serial.println(F(" seconds ago"));
      webSocket.disconnect();
    }
  }
}

// Send RFID scan to server
void sendRFIDScan(const String &codeStr) {
  JsonDocument doc;
  doc["type"]        = "rfid_scan";
  doc["resource_id"] = RESOURCE_ID;
  doc["rfid_code"]   = codeStr;
  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
  Serial.println(F("[RFID] Sent scan to server"));
}

// Send session end to server
void sendSessionEnd(const String &sessionId) {
  JsonDocument doc;
  doc["type"]        = "session_end";
  doc["resource_id"] = RESOURCE_ID;
  doc["session_id"]  = sessionId;
  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
}
