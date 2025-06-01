# ESP32-S3 Animated Clock Display

A colorful animated clock display for ESP32-S3 using an ST7789 display. Features include:

- Digital clock with time, date, and day of week
- Weather display with animated icons
- Temperature and humidity monitoring
- Colorful UI with animated elements

## Hardware Requirements

- ESP32-S3 DevKit
- ST7789 240x240 Display
- Connections:
  - Display BL → 3.3V
  - Display CS → GPIO15
  - Display DC → GPIO16
  - Display RST → GPIO17
  - Display SDA (MOSI) → GPIO13
  - Display SCL (SCLK) → GPIO14
  - Display VCC → 3.3V
  - Display GND → GND

## Software Setup

1. Ensure you have PlatformIO installed
2. Clone this repository
3. Update the following in `include/config.h`:
   - WiFi SSID and password
   - OpenWeatherMap API key (get one at [openweathermap.org](https://openweathermap.org/))
   - Timezone settings

## Features

- Modular code structure with separate components for display, WiFi, time, and weather
- Time synchronization via NTP
- Weather updates via OpenWeatherMap API
- Animated weather icons and UI elements
- Customizable color scheme

## Project Structure

- `main.cpp` - Main program entry point
- `display.cpp/.h` - Display driver and drawing functions
- `wifi_manager.cpp/.h` - WiFi connection handling
- `time_manager.cpp/.h` - Time synchronization and formatting
- `weather.cpp/.h` - Weather data fetching and parsing
- `ui.cpp/.h` - User interface rendering and animations
- `config.h` - Project configuration and settings 