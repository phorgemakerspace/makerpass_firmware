/*
 * MakerPass Configuration File
 * 
 * Configure your WiFi credentials, API settings, and device parameters here.
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// API Configuration
const char* API_URL = "https://your-moodle-site.com/local/makerpass";
const char* API_KEY = "your-api-key-here";
const char* EQUIPMENT_ID = "1";

// Master RFID Key (works offline for emergency access)
const char* MASTER_RFID_KEY = "FFFFFFFF";  // Replace with your master card ID (8 hex digits)

// Device Configuration
const char* DEVICE_TYPE = "door";  // "door" or "machine"

#endif
