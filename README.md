# MakerPass Firmware

![License](https://img.shields.io/badge/license-AGPLv3-blue.svg)  
![Build Status](https://img.shields.io/badge/status-active-green.svg)  
![Contributions Welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg) 

ESP32-based RFID access control system for makerspaces, supporting both door locks and machine control with real-time server communication.

## Overview

MakerPass is a comprehensive access control firmware designed for ESP32 microcontrollers. It provides secure, server-authenticated access to doors and machines using RFID cards, with real-time status monitoring and session management.

### Key Features

- **RFID Access Control**: Wiegand protocol RFID card reader support
- **Dual Device Types**: Door locks (timed access) and machine control (persistent sessions)
- **Real-time Communication**: WebSocket SSL connection to MakerPass server
- **Visual Status Display**: 1.9" TFT display with connection indicators and user feedback
- **Session Management**: Runtime tracking for machine usage with automatic timeout
- **WiFi Connectivity**: Automatic connection with status monitoring
- **Security**: SSL/TLS encrypted communication with NTP time synchronization

## Hardware Requirements

- **MakerPass PCB** (or compatible ESP32 development board)
- **1.9" ST7789 TFT Display** (170x320 pixels)
- **Wiegand RFID Reader** (26-bit format)
- **External WiFi Antenna** (for wireless internet)

### Pin Configuration

| Component | ESP32 Pin |
|-----------|-----------|
| TFT Display (MOSI) | GPIO 21 |
| TFT Display (SCLK) | GPIO 22 |
| TFT Display (DC) | GPIO 14 |
| TFT Display (RST) | GPIO 13 |
| RFID Data 0 | GPIO 4 |
| RFID Data 1 | GPIO 5 |
| RFID LED | GPIO 2 |
| RFID Beep | GPIO 16 |
| Relay Control | GPIO 17 |
| Relay LED | GPIO 18 |
| RFID Activity LED | GPIO 19 |

## Installation

### Prerequisites

1. **PlatformIO IDE** (recommended) or Arduino IDE
2. **ESP32 board package** installed

### Setup Instructions

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd makerpass_firmware
   ```

2. **Install PlatformIO** (if not already installed):
   ```bash
   # Recommened: Using VS Code extension
   # Install "PlatformIO IDE" extension in VS Code

   # Or using pip
   pip install platformio
   
   ```

3. **Build and upload:**
   ```bash
   # If using VS Code extension
   # press the blue arrow at the
   # bottom of the screen

   # Otherwise, follow these steps:

   # Build the project
   pio run
   
   # Upload to ESP32 (ensure device is connected via USB)
   pio run --target upload
   
   # Monitor serial output
   pio device monitor
   ```

### Configuration

Create or modify `include/config.h` with your specific settings:

```cpp
// WiFi Configuration
static const char* WIFI_SSID = "YourWiFiNetwork";
static const char* WIFI_PASSWORD = "YourWiFiPassword";

// Server Configuration  
static const char* WEBSOCKET_HOST = "your-server.com";
static const uint16_t WEBSOCKET_PORT = 443;
static const char* WEBSOCKET_PATH = "/ws";

// Device Configuration
static const char* DEVICE_TYPE = "door";  // or "machine"
static const char* RESOURCE_ID = "unique-resource-id";
static const char* API_KEY = "secret-key";
```

## Operation

### Device Types

#### Door Mode
- **Timed Access**: Relay activates for 5 seconds when access is granted
- **Immediate Response**: Shows "Access Granted" then returns to idle
- **Security**: Automatically locks after timeout

#### Machine Mode  
- **Session Control**: Relay remains active for entire usage session
- **Runtime Tracking**: Displays real-time usage timer
- **Card Presence**: Optionally requires card to remain present

### User Interface

The 1.9" TFT display shows:

```
┌─────────────────────────────────┐
│ Device Name                     │ ← Top status bar
├─────────────────────────────────┤
│                                 │
│ Ready                           │ ← Main message area
│ Scan card                       │   (or runtime display)
│                                 │
├─────────────────────────────────┤
│ WiFi ● Server ●                 │ ← Connection indicators
└─────────────────────────────────┘
```

**Status Indicators:**
- **Green dots**: Connected/authenticated
- **Red dots**: Disconnected/offline
- **Runtime Display**: Shows user name and elapsed time for machine sessions

### Server Communication

- **WebSocket SSL**: Secure real-time communication
- **JSON Protocol**: Structured message format
- **Keep-alive**: Automatic ping/pong every 5 minutes
- **Auto-reconnect**: Handles connection failures gracefully

## Development

### Project Structure

```
makerpass_firmware/
├── include/
│   ├── config.h             # Device configuration
│   ├── constants.h          # Display and timing constants  
│   ├── pins.h               # GPIO pin definitions
│   ├── ui_manager.h         # Display interface
│   ├── wifi_manager.h       # WiFi management
│   ├── websocket_manager.h  # Server communication
│   └── session_manager.h    # Access control logic
├── src/
│   ├── main.cpp             # Main program loop
│   ├── ui_manager.cpp       # Display rendering
│   ├── wifi_manager.cpp     # WiFi connection handling
│   ├── websocket_manager.cpp# WebSocket SSL communication
│   └── session_manager.cpp  # Relay and session control
└── platformio.ini           # Build configuration
```

### Key Libraries

- **TFT_eSPI**: High-performance display driver
- **ArduinoJson v7**: JSON message parsing
- **WebSockets**: SSL WebSocket client
- **Wiegand**: RFID reader protocol support

### Customization

- **Display Layout**: Modify `ui_manager.cpp` for different screen arrangements
- **Access Logic**: Update `session_manager.cpp` for custom access rules  
- **Network Protocol**: Extend `websocket_manager.cpp` for additional server messages
- **Hardware Pins**: Adjust `pins.h` for different board configurations

## Troubleshooting

### Common Issues

**Display not working:**
- Verify TFT wiring matches pin configuration
- Check `platformio.ini` TFT_* settings

**RFID not reading:**
- Confirm Wiegand wiring (Data0/Data1)
- Check card format compatibility (26-bit/34-bit) (125khz)
- Verify reader power supply

**WiFi connection fails:**
- Double-check SSID/password in `config.h`
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength at device location

**Server connection issues:**
- Verify WebSocket URL and credentials
- Check firewall/network restrictions
- Monitor serial output for SSL errors

### Serial Debugging

Enable serial monitoring at 115200 baud to see detailed logs:
```
[WIFI] Connecting to WiFi...
[WIFI] Connected. IP: 192.168.1.100
[WS] Connecting to server...
[WS] WebSocket connected
[RFID] Card scanned: 12345678
[SESSION] Started for user: John Doe
```

## License

This project is open source. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes with proper testing
4. Submit a pull request with clear description

## Support

For technical support or feature requests, please open an issue in the project repository.
