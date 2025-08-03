# MakerPass Firmware

This firmware is designed for ESP32-based MakerPass hardware to control access to machines and doors using an RFID reader, interfacing with the MakerPass plugin for Moodle.

## ⚠️ IMPORTANT: WiFi Requirements

**ESP32 only supports 2.4GHz WiFi networks**

- Your WiFi network must be 2.4GHz (not 5GHz)
- Most routers broadcast both frequencies - look for networks without "5G" in the name
- Common 2.4GHz network names: "MyNetwork", "MyNetwork_2.4G"
- Common 5GHz network names: "MyNetwork_5G", "MyNetwork_5GHz"

## Hardware Requirements

- **ESP32-WROVER-IE** module
- **ST7789 1.9" TFT Display** (170x320 pixels)
- **Wiegand RFID Reader** (125kHz/13.56MHz compatible)
- **Relay module** for controlling doors/machines
- **Status LEDs** (WiFi, Relay, RFID)

## Pin Configuration

The following pin assignments are based on the hardware schematic:

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
git clone <repository-url>
cd MakerPass
```

### 2. Project Structure
```
MakerPass/
├── platformio.ini          # PlatformIO configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Main firmware code
│   └── config.h            # WiFi and API configuration
└── .gitignore             # Git ignore file
```

### 3. Configuration Setup

Create or edit `src/config.h` with your specific settings:

```cpp
/*
 * MakerPass Configuration File
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
const char* WIFI_SSID = "Your_WiFi_Network";
const char* WIFI_PASSWORD = "Your_WiFi_Password";

// API Configuration
const char* API_URL = "https://your-server.com";
const char* API_KEY = "your_api_key_here";
const char* EQUIPMENT_ID = "1";

// Default settings (used if not available from API)
const unsigned long DEFAULT_DOOR_OPEN_TIME = 5000;  // 5 seconds in milliseconds

#endif
```

## Build and Upload Instructions

### Using VS Code with PlatformIO Extension (Recommended)

#### 1. Install VS Code Extensions
- Install **VS Code**
- Install **PlatformIO IDE** extension from the marketplace

#### 2. Open Project
- File → Open Folder → Select the `MakerPass` directory
- PlatformIO will automatically detect the `platformio.ini` file

#### 3. Install Dependencies
Libraries will be automatically installed based on `platformio.ini`:
- ArduinoJson (JSON parsing)
- Adafruit GFX Library (graphics)
- Adafruit ST7789 Library (display driver)
- Wiegand Protocol Library (RFID reader)

#### 4. Build Project
- Click the **checkmark (✓)** in the PlatformIO toolbar
- Or press `Ctrl+Alt+B` (Windows/Linux) or `Cmd+Alt+B` (macOS)
- Or `Cmd+Shift+P` → "PlatformIO: Build"

#### 5. Upload to ESP32
- Connect ESP32 via USB cable
- Click the **arrow (→)** in the PlatformIO toolbar
- Or press `Ctrl+Alt+U` (Windows/Linux) or `Cmd+Alt+U` (macOS)
- Or `Cmd+Shift+P` → "PlatformIO: Upload"

#### 6. Monitor Serial Output
- Click the **plug icon** in the PlatformIO toolbar
- Or press `Ctrl+Alt+S` (Windows/Linux) or `Cmd+Alt+S` (macOS)
- Serial monitor will open at 115200 baud

## Expected Boot Sequence

When the firmware boots successfully, you should see:

### Serial Monitor Output
```
MakerPass Starting...
Setting up display...
Initializing display...
Display initialized successfully
MakerPass Booting...
Connecting to WiFi...
SSID: Your_WiFi_Network
WiFi connected successfully!
IP address: 192.168.1.xxx
Signal strength: -xx dBm
Retrieving resource configuration...
API URL: https://your-server.com/api/resource/1
HTTP Response Code: 200
Resource configuration retrieved successfully
Setting up RFID...
Initializing RFID reader...
RFID D0 pin: 4
RFID D1 pin: 5
RFID reader initialized successfully
Setup complete!
```

### Display Output
1. **"MakerPass Booting..."** - Initial startup
2. **"WiFi connecting..."** - Network connection attempt
3. **"WiFi Connected!"** - Successful connection (green text)
4. **"Getting config..."** - API configuration retrieval
5. **"Config loaded!"** - Successful API response (green text)
6. **Main interface** - Shows device status, WiFi info, and instructions

### Status LEDs
- **WiFi LED (GPIO 27)** - Solid when connected to WiFi
- **Relay LED (GPIO 26)** - On when relay is active
- **RFID LED (GPIO 25)** - Flashes when card is read

## Troubleshooting

### Build Errors

#### Library Not Found
```bash
# Manually install missing libraries
pio lib install "bblanchon/ArduinoJson"
pio lib install "adafruit/Adafruit GFX Library"
pio lib install "adafruit/Adafruit ST7735 and ST7789 Library"
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

#### API Connection Issues
- Verify API_URL and API_KEY in `config.h`
- Check server is accessible from device's network
- Monitor serial output for HTTP response codes

### Common Error Codes

- **WiFi Status 6** - Wrong password
- **WiFi Status 1** - No SSID found
- **HTTP 401** - Invalid API key
- **HTTP 404** - Invalid API endpoint
- **HTTP 500** - Server error

## API Integration

The device communicates with the MakerPass Moodle plugin via REST API:

### Endpoints Used
- `GET /api/resource/{id}` - Retrieve device configuration
- `POST /api/check_access` - Validate RFID card access

### Expected API Responses

#### Resource Configuration
```json
{
  "name": "Front Door",
  "type": "door",
  "relay_time": 5,
  "card_present_required": true
}
```

#### Access Check
```json
{
  "status": "granted",
  "user": "john.doe"
}
```

## Development Notes

### Code Structure
- `setup()` - Hardware initialization and configuration
- `loop()` - Main program loop, handles RFID reads and status updates
- `connectToWifi()` - WiFi connection management
- `retrieveResourceConfig()` - API configuration retrieval
- `handleRFID()` - Process RFID card reads
- `updateDisplay()` - Update TFT display content

### Key Features
- **Automatic WiFi reconnection** - Handles network drops
- **API fallback** - Works with defaults if API unavailable
- **Visual feedback** - Status LEDs and display messages
- **Serial debugging** - Comprehensive logging output
- **Card presence detection** - For machines requiring continuous card presence

### Board Configuration
The `platformio.ini` file is configured for Generic ESP32. Modify the `board` setting if using a different ESP32 variant:

```ini
board = esp32dev           # Generic ESP32
board = esp32-wrover-kit   # ESP32-WROVER
board = esp32s3dev         # ESP32-S3
```

## Support

For technical support or questions:
- Check the troubleshooting section above
- Review serial monitor output for error details
- Verify hardware connections match pin definitions


modify the auto-flash circuit. You can:

Add a pull-up resistor (10kΩ) from GPIO0 to 3.3V
Add a reset capacitor (100nF) from ENABLE to GND
Modify the auto-flash circuit to be less aggressive
Option 4: Bypass Auto-Flash
If the auto-flash circuit is causing problems, you can temporarily disable it by:

Removing or lifting resistors R12/R13 from the board
This will require manual BOOT+ENABLE button presses for programming, but eliminate power-cycle issues
