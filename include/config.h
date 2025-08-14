// Configuration parameters for the MakerPass firmware.
//
// This header centralises all tunable values so that
// end users can adapt the firmware without digging into
// the core logic.  WiFi credentials, WebSocket endpoint,
// API keys and device identifiers live here.  A master
// RFID card code is also defined for emergency use when
// the network is unavailable.

#pragma once

#include <Arduino.h>

// WiFi credentials.  Replace these with the SSID and
// password for the local network.
static const char* WIFI_SSID     = "U+Net430C";
static const char* WIFI_PASSWORD = "6H4E6C840#";

// WebSocket server details.  The host must not include
// the "ws://" prefix and should match the hostname or IP
// address of the access control API.  The port is the
// TCP port used by the server (default 3000).  The path
// identifies the WebSocket endpoint, typically "/ws".
static const char* WS_HOST = "makerpass.phorgemakerspace.com";
static const uint16_t WS_PORT = 443;
static const char* WS_PATH = "/ws";

// API key used for authenticating this device.  The
// dashboard generates these keys; each device must have
// its own key.  Never commit real keys to a public
// repository.
static const char* API_KEY = "5618e7db30c5940fc07182ad6e92d89c22b452d26cd8eff5c88886a939d8f0af";

// Unique identifier for the resource this device controls.
// This identifier corresponds to an entry configured on 
// the dashboard. For example, a door could have id "door‑001" 
// and a machine could have id "machine‑A".
static const char* RESOURCE_ID = "ERNPTY";

// The type of device: "door" or "machine".  Doors energise
// the relay for a fixed period; machines require
// start/stop control and may require the card to remain
// present.  Choose the appropriate type for your
// installation.
static const char* DEVICE_TYPE = "machine";

// Duration (in milliseconds) to energise the relay when
// granting door access.  This is ignored for machine
// devices where the relay remains on for the duration of
// a session.
static const uint32_t RELAY_DOOR_DURATION_MS = 5000;

// Master RFID card code.  This eight‑character hex
// string provides an emergency override when the
// WebSocket connection is down.  It should match the
// code printed on a physical card configured as the
// master in the original firmware.  Keep it secret.
static const char* MASTER_KEY = "01234567";