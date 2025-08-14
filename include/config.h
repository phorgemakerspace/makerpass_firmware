// Configuration parameters for the MakerPass firmware.

// This header centralises all tunable values so that
// end users can adapt the firmware without digging into
// the core logic. A master RFID card code is also defined
// for emergency use when the network is unavailable.

// All values in this file are examples and should be replaced
// with real values for your installation.  Do not commit
// real credentials or keys to a public repository.

#pragma once

#include <Arduino.h>

// WiFi credentials.  Replace these with the SSID and
// password for the local network. Note: ESP32 does not
// support 5 GHz networks, so use a 2.4 GHz band.
static const char* WIFI_SSID     = "your-ssid";
static const char* WIFI_PASSWORD = "your-password";

// WebSocket server details. If using the makerpass dashboard,
// just use the URL that points to the home page. Do not include
// the "wss://" prefix and does not work with unsecure connections.
// The path identifies the WebSocket endpoint, typically "/ws".
static const char* WS_HOST = "yourdomain.com";
static const uint16_t WS_PORT = 443;
static const char* WS_PATH = "/ws";

// API key used for authenticating this device. The makerpass
// dashboard generates this keys; Never commit real keys to a
// public repository, the below key is just an example.
static const char* API_KEY = "1234567890-your-api-key-1234567890";

// Unique identifier for the resource this device controls.
// This identifier corresponds to an entry configured on 
// the dashboard. This is an eight-character alphanumeric string.
static const char* RESOURCE_ID = "ABCD1234";

// The type of device: "door" or "machine". Doors energise
// the relay for a fixed period; machines require
// start/stop control and may require the card to remain
// present. Choose the appropriate type for your
// installation.
static const char* DEVICE_TYPE = "machine";

// Duration (in milliseconds) to energise the relay when
// granting door access. This is ignored for machine
// devices where the relay remains on for the duration of
// a session. This usually doesn't need to be changed.
static const uint32_t RELAY_DOOR_DURATION_MS = 5000;

// Master RFID card code. This eight‑character hex
// string provides an override, even when the
// WebSocket connection is down. Keep it secure.
static const char* MASTER_KEY = "A1B2C3D4";