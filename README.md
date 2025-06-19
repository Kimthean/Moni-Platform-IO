# ESP32-S3 Moni Platform - Spotify Music Player & Weather Display

A comprehensive ESP32-S3 project featuring Spotify music control, weather display, digital clock, and audio capabilities using LVGL UI framework, ST7789 display, I2S audio components, and interactive buttons.

## Features

### 🎵 Spotify Integration
- **Music Control**: Play, pause, skip tracks, and volume control
- **Track Display**: Current song, artist, album, and playback progress
- **Album Art**: Dynamic album artwork display with smooth loading
- **Spotify API**: OAuth authentication with automatic token refresh

### 🌤️ Weather & Time
- **Weather Display**: Real-time weather data with animated icons
- **Digital Clock**: Time, date, and day of week display
- **Timezone Support**: Configurable timezone settings

### 🎵 Audio System
- **Audio Visualization**: Real-time audio spectrum display
- **Voice Recording**: Hold-to-record functionality
- **I2S Audio**: High-quality audio input/output

### 🖥️ User Interface
- **LVGL Framework**: Modern, responsive UI with smooth animations
- **Multi-Screen Layout**: Dedicated screens for Spotify, weather, and settings
- **Interactive Controls**: Button-controlled navigation and settings
- **WiFi Management**: SmartConfig and stored credentials support

## Hardware Requirements

### Core Components
- **ESP32-S3 DevKit** (4D Systems ESP32-S3 Gen4 R8N16)
- **ST7789 240x320 TFT Display**
- **MAX98357A I2S Audio Amplifier**
- **INMP441 I2S Microphone**
- **3 Push Buttons**
- **Speaker** (4-8Ω, 3W recommended)

## Hardware Wiring

### Display Connections (ST7789)

| Display Pin | ESP32-S3 GPIO | Description |
|-------------|---------------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| CS | GPIO 15 | Chip Select |
| DC | GPIO 16 | Data/Command |
| RST | GPIO 17 | Reset |
| SDA (MOSI) | GPIO 13 | Serial Data |
| SCL (SCLK) | GPIO 14 | Serial Clock |
| BL | 3.3V | Backlight (always on) |

### Display Configuration

The project uses the **TFT_eSPI** library with the following configuration:

#### Driver Settings
- **Driver**: ST7789_DRIVER
- **Resolution**: 240x320 pixels
- **Color Order**: BGR (Blue-Green-Red)
- **SPI Interface**: Hardware SPI using HSPI port

### I2S Audio Bus Configuration

The microphone and amplifier share clock signals but use separate data pins:

**Shared Clock Signals:**
| I2S Signal | ESP32-S3 GPIO | MAX98357A Pin | INMP441 Pin | Description |
|------------|---------------|---------------|-------------|-------------|
| BCLK | GPIO 14 | BCLK | SCK | Bit Clock |
| LRCLK (WS) | GPIO 15 | LRC | WS | Left/Right Clock |

**Separate Data Pins:**
| Device | ESP32-S3 GPIO | Device Pin | Direction | Description |
|--------|---------------|------------|-----------|-------------|
| INMP441 Microphone | GPIO 13 | SD | Input | Audio data from microphone |
| MAX98357A Amplifier | GPIO 21 | DIN | Output | Audio data to amplifier |

#### MAX98357A Audio Amplifier

| Amplifier Pin | Connection | Description |
|---------------|------------|-------------|
| VIN | 5V or 3.3V | Power supply |
| GND | GND | Ground |
| DIN | GPIO 21 | Audio data input |
| BCLK | GPIO 14 | Bit clock |
| LRC | GPIO 15 | Left/Right clock |
| GAIN | Leave floating | Default gain |
| SD | 3.3V | Shutdown (high = enabled) |
| + Speaker | Speaker + | Positive terminal |
| - Speaker | Speaker - | Negative terminal |

#### INMP441 I2S Microphone

| Microphone Pin | Connection | Description |
|----------------|------------|-------------|
| VDD | 3.3V | Power supply |
| GND | GND | Ground |
| SD | GPIO 13 | Serial data output |
| WS | GPIO 15 | Word select |
| SCK | GPIO 14 | Serial clock |
| L/R | GND | Left channel select |

### Button Connections

| Button | ESP32-S3 GPIO | Function |
|--------|---------------|----------|
| Button 1 | GPIO 1 | Play/Pause / Context actions |
| Button 2 | GPIO 2 | Next Track / Volume Up (hold) |
| Button 3 | GPIO 3 | Previous Track / Volume Down (hold) |

**Wiring**: Connect one side to GPIO, other side to GND. Internal pull-ups enabled.

#### Button Functions by Screen:
- **Spotify Screen**: Music playback control (play/pause, next/previous, volume)
- **Weather Screen**: Display brightness adjustment
- **All Screens**: Long press Button 3 to switch between screens

## Software Setup

### 1. Environment Variables Configuration

The project uses environment variables to securely manage API keys and credentials:

1. **Create a `.env` file** in the project root with your credentials:
```env
# WiFi Configuration
WIFI_SSID=Your_WiFi_Network
WIFI_PASSWORD=Your_WiFi_Password

# Weather API (get from openweathermap.org)
WEATHER_API_KEY=your_openweather_api_key

# Spotify API (get from developer.spotify.com)
SPOTIFY_CLIENT_ID=your_spotify_client_id
SPOTIFY_CLIENT_SECRET=your_spotify_client_secret
SPOTIFY_REFRESH_TOKEN=your_spotify_refresh_token
```

2. **The `.env` file is git-ignored** for security - never commit your secrets!

### 2. Spotify API Setup

#### Get Spotify Credentials:
1. Go to [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Create a new app: "ESP32 Spotify Player"
3. Note your **Client ID** and **Client Secret**
4. Add redirect URI: `http://localhost:3000/callback`

#### Get OAuth Tokens:
1. Navigate to the `spotify-auth-server` directory
2. Install dependencies: `npm install`
3. Create `.env` file with your Spotify credentials
4. Run the auth server: `npm start`
5. Open http://localhost:3000 and authorize your app
6. Copy the refresh token to your main project's `.env` file

### 3. Weather API Setup

1. Sign up at [OpenWeatherMap](https://openweathermap.org/api)
2. Get your free API key
3. Add it to your `.env` file as `WEATHER_API_KEY`

### 4. PlatformIO Configuration

The project uses PlatformIO build flags to inject environment variables:

```ini
build_flags = 
    -DWIFI_SSID=\"${sysenv.WIFI_SSID}\"
    -DWIFI_PASSWORD=\"${sysenv.WIFI_PASSWORD}\"
    -DWEATHER_API_KEY=\"${sysenv.WEATHER_API_KEY}\"
    -DSPOTIFY_CLIENT_ID=\"${sysenv.SPOTIFY_CLIENT_ID}\"
    -DSPOTIFY_CLIENT_SECRET=\"${sysenv.SPOTIFY_CLIENT_SECRET}\"
    -DSPOTIFY_REFRESH_TOKEN=\"${sysenv.SPOTIFY_REFRESH_TOKEN}\"
```

## Audio Configuration

### I2S Bus Sharing
The system dynamically switches the I2S bus between:
- **TX Mode**: Audio output (44.1kHz, 16-bit)
- **RX Mode**: Audio input (16kHz, 32-bit for voice)

## UI Framework

### LVGL Integration
- **Version**: 8.3.11
- **Display Driver**: Custom ST7789 driver
- **Input**: Button-based navigation
- **Themes**: Dark theme optimized for music visualization
- **Animations**: Smooth transitions and loading indicators

### Screen Layout
- **Spotify Screen**: Track info, album art, playback controls
- **Weather Screen**: Current conditions, temperature, forecast
- **Device Screen**: System information and settings

## Project Structure

```
Moni Platform IO/
├── src/                          # Main source code
│   ├── main.cpp                  # Application entry point
│   ├── spotify_manager.cpp       # Spotify API integration
│   ├── lvgl_*.cpp                # LVGL UI components
│   ├── weather.cpp               # Weather data fetching
│   ├── wifi_manager.cpp          # WiFi connection management
│   └── audio_manager.cpp         # Audio system control
├── include/                      # Header files
│   ├── config.h                  # Configuration constants
│   ├── spotify_manager.h         # Spotify API definitions
│   ├── lvgl_*.h                  # LVGL UI headers
│   └── User_Setup.h              # TFT_eSPI configuration
├── spotify-auth-server/          # Node.js OAuth server
│   ├── server.js                 # Express server for Spotify auth
│   ├── package.json              # Node.js dependencies
│   └── .env                      # Server environment variables
├── platformio.ini                # PlatformIO configuration
├── .env                          # Main project secrets
└── README.md                     # This file
```

## Usage

### First Time Setup

1. **Clone the repository**
2. **Set up environment variables** (see Software Setup section)
3. **Install PlatformIO** and open the project
4. **Configure Spotify authentication** using the auth server
5. **Build and upload** to your ESP32-S3

### Operation

1. **Power on** the device
2. **WiFi Setup**: 
   - If no credentials stored, SmartConfig will start automatically
   - Use your phone's WiFi settings to configure
3. **Screen Navigation**:
   - Long press Button 3 to cycle through screens
   - Use buttons for context-specific actions on each screen
4. **Spotify Control**:
   - Ensure Spotify is playing on another device first
   - Use buttons to control playback
   - Album art and track info will display automatically

### Button Controls

#### Spotify Screen
- **Button 1**: Play/Pause current track
- **Button 2**: Next track (short press) / Volume up (long press)
- **Button 3**: Previous track (short press) / Volume down (long press)

#### Weather Screen
- **Button 1**: Adjust display brightness
- **Button 2**: Refresh weather data
- **Button 3**: Switch to next screen (long press)

## Building and Uploading

### Prerequisites
- **PlatformIO IDE** or **PlatformIO Core**
- **ESP32-S3 USB drivers**
- **Node.js** (for Spotify auth server)

### Build Steps

1. **Install dependencies**:
   ```bash
   # For Spotify auth server
   cd spotify-auth-server
   npm install
   cd ..
   ```

2. **Set up environment variables**:
   ```bash
   # Create .env file with your secrets
   cp .env.example .env
   # Edit .env with your actual credentials
   ```

3. **Build and upload**:
   ```bash
   # Using PlatformIO CLI
   pio run --target upload --target monitor
   
   # Or use PlatformIO IDE build/upload buttons
   ```

### Troubleshooting

#### WiFi Connection Issues
- Ensure SmartConfig is supported on your router
- Try connecting to a 2.4GHz network first
- Check that WiFi credentials are correct in `.env`

#### Spotify Not Working
- Verify Spotify Premium account (required for API access)
- Check that Spotify is playing on another device
- Ensure OAuth tokens are valid (re-run auth server if needed)
- Verify environment variables are set correctly

#### Display Issues
- Check TFT_eSPI configuration in `User_Setup.h`
- Verify wiring matches the pin configuration
- Ensure adequate power supply (3.3V/5V as needed)

## Dependencies

### PlatformIO Libraries
- `lvgl/lvgl@^8.3.11` - UI framework
- `bodmer/TFT_eSPI@^2.5.43` - Display driver
- `bblanchon/ArduinoJson@^6.21.5` - JSON parsing
- `arduino-libraries/NTPClient@^3.2.1` - Time synchronization
- `earlephilhower/ESP8266Audio@^1.9.7` - Audio processing
- `spotify-api-arduino` - Spotify API client
- `bodmer/TJpg_Decoder@^1.0.8` - JPEG decoding

### Node.js Dependencies (Auth Server)
- `express` - Web server framework
- `axios` - HTTP client
- `dotenv` - Environment variable management

## Contributing

1. **Fork the repository**
2. **Create a feature branch**
3. **Make your changes**
4. **Test thoroughly**
5. **Submit a pull request**

## License

This project is open source. Please check the license file for details.

## Acknowledgments

- **LVGL** - Excellent embedded graphics library
- **Spotify Web API** - Music streaming integration
- **OpenWeatherMap** - Weather data provider
- **PlatformIO** - Development platform
- **TFT_eSPI** - Efficient display driver

---

**Enjoy your ESP32-S3 Spotify Music Player! 🎵**

