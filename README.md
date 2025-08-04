# MakerPass Firmware

This is firmware for custom ESP32-based MakerPass hardware to control access to machines and doors using RFID authentication, with full integration to the MakerPass plugin for Moodle.

## Key Features

- **Smart Device Types**: Automatic behavior for doors (timed unlock) vs machines (toggle on/off)
- **Visual Feedback**: 1.9" TFT display with real-time status and runtime tracking
- **Robust Connectivity**: Auto-reconnecting WiFi with graceful degradation
- **Master Key Access**: Emergency offline access even when API/WiFi unavailable

## ⚠️ IMPORTANT: WiFi Requirements

**ESP32 only supports 2.4GHz WiFi networks**

- Your WiFi network must be 2.4GHz (not 5GHz)
- Most routers broadcast both frequencies - look for networks without "5G" in the name
- Common 2.4GHz network names: "MyNetwork", "MyNetwork_2.4G"
- Common 5GHz network names: "MyNetwork_5G", "MyNetwork_5GHz"

## Bill of Materials

- **MakerPass Baord** ESP32-WROVER-IE based PCB
- **ST7789 1.9" TFT Display** (170x320 pixels)
- **Wiegand RFID Reader** (125kHz/13.56MHz compatible)
- **WiFi Antenna** Mini-PCIe type 2 connector

## Pin Configuration

The following pin assignments are based on the our custom hardware:

### Display (ST7789 TFT)
- `TFT_DC` = GPIO 14
- `TFT_RESET` = GPIO 13
- `TFT_MOSI` = GPIO 21 (SDA)
- `TFT_SCLK` = GPIO 22 (SCL)
- `TFT_CS` = Not used (-1)

### RFID Reader (Wiegand)
- `RFID_D0` = GPIO 4
- `RFID_D1` = GPIO 5
- `RFID_LED` = GPIO 18
- `RFID_BEEP` = GPIO 19

### Control & Status
- `RELAY_PIN` = GPIO 23
- `LED_WIFI` = GPIO 27 (WiFi status)
- `LED_RELAY` = GPIO 26 (Relay status)
- `LED_RFID` = GPIO 25 (RFID status)

## Software Requirements

### Development Environment
- **VS Code** with **PlatformIO IDE Extension**
- **Git** (for cloning repositories)

## Project Setup

### 1. Clone or Download Project
```bash
git clone https://github.com/phorgemakerspace/makerpass_firmware.git
cd makerpass_firmware
```

### 2. Project Structure
```
MakerPass/
├── platformio.ini          # PlatformIO configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Main firmware code
│   └── config.example.h    # WiFi and API configuration
└── .gitignore              # Git ignore file
```

### 3. Configuration Setup

Create your configuration file:

1. Copy `src/config.example.h` to `src/config.h` (or create a new one)
2. Edit `src/config.h` with your specific settings:

```cpp
/*
 * MakerPass Configuration File
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration  
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// API Configuration
const char* API_URL = "https://your-moodle-site.com";
const char* API_KEY = "your-api-key-here";
const char* EQUIPMENT_ID = "1";

// Master RFID Key (emergency offline access)
const char* MASTER_RFID_KEY = "FFFFFFFF";  // Replace with your master card ID

// Device Configuration (source of truth)
const char* DEVICE_TYPE = "door";  // "door" or "machine"

#endif
```

**Important Notes:**
- **Master Key**: Use 8 hex digits (e.g., "00CF195F") - works offline for emergency access
- **Device Type**: "door" = timed unlock, "machine" = toggle on/off behavior (or card present behavior)

## Build and Upload Instructions

### Using VS Code with PlatformIO Extension (Recommended)

#### 1. Install VS Code Extensions
- Install **VS Code**
- Install **PlatformIO IDE** extension from the marketplace

#### 2. Open Project
- File → Open Folder → Select the `makerpass_firmware` directory
- PlatformIO will automatically detect the `platformio.ini` file

#### 3. Install Dependencies
Libraries will be automatically installed based on `platformio.ini`:
- **ArduinoJson** v7.0.4+ (JSON parsing for API communication)
- **TFT_eSPI** v2.5.43+ (Main TFT display library with optimized ESP32 support)
- **Wiegand Protocol Library** (RFID reader communication - GitHub source)

#### 4. Build Project
- Click the **checkmark (✓)** in the PlatformIO toolbar
- Or press `Ctrl+Alt+B` (Windows/Linux) or `Cmd+Alt+B` (macOS)
- Or `Cmd+Shift+P` → "PlatformIO: Build"

#### 5. Upload to ESP32
- Connect MakerPass board via USB cable
- Click the **arrow (→)** in the PlatformIO toolbar
- Or press `Ctrl+Alt+U` (Windows/Linux) or `Cmd+Alt+U` (macOS)
- Or `Cmd+Shift+P` → "PlatformIO: Upload"

#### 6. Monitor Serial Output
- Click the **plug icon** in the PlatformIO toolbar
- Or press `Ctrl+Alt+S` (Windows/Linux) or `Cmd+Alt+S` (macOS)
- Serial monitor will open at 115200 baud

### Serial Monitor Output
```
=== MAKERPASS SYSTEM STARTUP ===
Initializing hardware...
LED test...
Device type: Door
Initializing display...
Display initialized successfully
Connecting to WiFi...
SSID: Your_WiFi_Network
WiFi connected successfully!
IP address: 192.168.1.xxx
Signal strength: -xx dBm
Retrieving resource configuration...
API URL: https://your-server.com/api/resource/1
HTTP Response Code: 200
Resource configuration retrieved successfully
Resource name: Front Door
Initializing RFID reader...
RFID reader initialized successfully
=== SETUP COMPLETE ===
```

### Display Sequence
1. **"MAKERPASS STARTING"**
2. **"WIFI CONNECTING"**
3. **"WIFI CONNECTED - Getting config"**
4. **"CONFIG LOADED"**
5. **"MAKERPASS READY"**

### Error Handling
- **WiFi Errors**: Shows "WIFI ERROR - Master key only"
- **API Errors**: Shows "CONFIG ERROR - Master key only"  
- **Master Key Mode**: Device remains functional with offline access

### Status LEDs
- **WiFi LED (GPIO 27)** - Solid when connected to WiFi
- **Relay LED (GPIO 26)** - On when relay is active
- **RFID LED (GPIO 25)** - Flashes when card is read

## Troubleshooting

### Build Errors

#### Library Not Found
```bash
# Manually install missing libraries if needed
pio lib install "bblanchon/ArduinoJson"
pio lib install "bodmer/TFT_eSPI"
```

#### Wiegand Library Issues
The Wiegand library is installed from GitHub. If issues occur:
1. Check internet connection
2. Try building again (library may be cached)

### Runtime Issues

#### Display Not Working
- Check TFT wiring matches pin definitions
- Verify power supply is adequate (ESP32 + display requires stable 3.3V)
- Check serial monitor for "Display initialized successfully"

#### WiFi Connection Fails
- Verify SSID and password in `config.h`
- Check WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Monitor serial output for specific error codes

#### RFID Not Responding
- Check Wiegand wiring (D0, D1 pins)
- Verify RFID reader power supply
- Test with known working RFID cards
- Check RFID card frequency (card should be 125kHz/13.56MHz)

#### API Connection Issues
- Verify API_URL and API_KEY in `config.h`
- API_URL should not have a trailing `/`
- Check server is accessible from device's network
- Monitor serial output for HTTP response codes

### Common Error Codes

- **WiFi Status 6** - Wrong password
- **WiFi Status 1** - No SSID found
- **HTTP 401** - Invalid API key
- **HTTP 404** - Invalid API endpoint
- **HTTP 500** - Server error

## API Integration

The device uses the updated MakerPass API specification:

### Endpoints Used
- `GET /api/makerpass.php/resource` - Retrieve device configuration
- `POST /api/makerpass.php/check_access` - Validate RFID card access

### Authentication
All requests use header-based authentication:
```
X-API-Key: YOUR_API_KEY
```

### API Request/Response

#### Resource Configuration Request
```
GET /api/makerpass.php/resource
Headers: 
  X-API-Key: your-api-key
  X-Resource-ID: 1
```

#### Resource Configuration Response
```json
{
  "resource_id": 1,
  "name": "Front Door",
  "card_present_required": true
}
```

#### Access Check Request  
```
POST /api/makerpass.php/check_access
Headers:
  Content-Type: application/json
  X-API-Key: your-api-key
  X-RFID: 00CF195F
  X-Resource-ID: 1
Body: (empty - data passed in headers)
```

#### Access Check Response
```json
{
  "status": "granted",
  "reason": "granted"
}
```

#### Access Denied Response
```json
{
  "status": "denied", 
  "reason": "invalid_rfid"
}
```

### Error Handling
The firmware handles these HTTP status codes:
- **400**: Bad request (invalid parameters)
- **401**: Unauthorized (invalid API key)
- **405**: Method not allowed (wrong endpoint)
- **500**: Server error

#### Error Response Format
```json
{
  "error": "Invalid API key"
}
```

### Configuration Architecture
- **API provides**: Device name, card presence requirements
- **Local config.h controls**: Device type (door/machine), WiFi, master key
- **Internal defaults**: Relay timing (5s for doors), timeout behavior (3s messages)

### Fallback Behavior
- **No WiFi/API**: Master key still works for emergency access
- **API errors**: Device displays error but remains functional with master key
- **Network drops**: Auto-reconnection with graceful degradation

## Development Notes

### Code Structure
- `setup()` - Hardware initialization, WiFi connection, API configuration
- `loop()` - RFID monitoring, WiFi management, relay control, display updates
- `handleRFID()` - Process card scans with visual feedback and access validation
- `checkAccess()` - Master key check first, then API validation with detailed logging
- `showMessage()` - Unified message display with automatic timeout handling
- `updateDisplay()` - Real-time runtime tracking for active machines
- `showMainScreen()` - Main interface with device status and instructions

### Device Behavior Differences

#### Door Mode (`DEVICE_TYPE = "door"`)
- **Single scan**: Unlocks door for configured time (default 5 seconds)
- **Automatic lock**: Relay deactivates after timer expires
- **Display**: Shows "ACCESS GRANTED - Door unlocked" then returns to main screen
- **LED feedback**: Relay LED on during unlock period

#### Machine Mode (`DEVICE_TYPE = "machine"`)  
- **Toggle behavior**: First scan activates, second scan deactivates
- **Runtime tracking**: Live display shows elapsed time (HH:MM:SS format)
- **Active display**: Stays on runtime screen while machine is active
- **User tracking**: Shows which card activated the machine

### Error Handling & Debugging
- **Comprehensive logging**: All RFID scans logged with card ID and decision rationale
- **API transparency**: Request/response data logged for troubleshooting
- **Master key validation**: Clear logging of master key match/mismatch
- **Network resilience**: Auto-reconnection with status feedback

## Production Status

This firmware is **BETA** with the following optimizations:

### ✅ Completed Features
- **Hardware Integration**: Full ESP32 + TFT + Wiegand RFID support
- **API Integration**: Simplified, robust communication with MakerPass backend
- **Master Key System**: Reliable offline emergency access
- **User Interface**: Informative display messaging
- **Device Types**: Smart door vs machine behavior with runtime tracking
- **Error Handling**: Graceful degradation and comprehensive logging

## Support

For technical support or questions:
- Check the troubleshooting section above
- Review serial monitor output for detailed error logging
- Verify hardware connections match pin definitions
- Test with master key for offline functionality

---

**MakerPass Firmware v0.1** - BETA
*ESP32-based RFID access control with Moodle integration*
