// WebSocket management header for MakerPass firmware

#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Function declarations
void initWebSocket();
void sendDeviceAuth();
void handleIncomingMessage(const String &message);
void processJsonMessage(JsonDocument &doc);
void handleWebSocketKeepAlive();
void sendRFIDScan(const String &codeStr);
void sendSessionEnd(const String &sessionId);
