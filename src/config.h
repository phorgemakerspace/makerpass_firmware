/*
 * MakerPass
 * Configuration File
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// API Configuration
extern const char* API_URL;
extern const char* API_KEY;
extern const char* EQUIPMENT_ID;

// Master RFID Key (works offline for emergency access)
extern const char* MASTER_RFID_KEY;

// Device Configuration (source of truth)
extern const char* DEVICE_TYPE;

// Advanced Settings
extern const unsigned long DOOR_CLOSE_TIME;
extern const unsigned long MESSAGE_TIMEOUT;

#endif

// API will return:
// {
//   "name": "Front Door",
//   "card_present_required": true
// }
//
// Device type comes from config.h above
// Relay timing is set internally in main.cpp