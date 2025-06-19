# ESP32-S3 Moni Platform - Weather Display & Audio System

A comprehensive ESP32-S3 project featuring weather display, audio visualization, and voice recording capabilities using an ST7789 display, I2S audio components, and interactive buttons.

## Features

- **Weather Display**: Real-time weather data with animated icons
- **Digital Clock**: Time, date, and day of week display
- **Audio Visualization**: Real-time audio spectrum display
- **Voice Recording**: Hold-to-record functionality
- **Interactive UI**: Button-controlled navigation and settings
- **WiFi Connectivity**: SmartConfig and stored credentials support

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
| CS | GPIO 10 | Chip Select |
| DC | GPIO 6 | Data/Command |
| RST | GPIO 7 | Reset |
| SDA (MOSI) | GPIO 11 | Serial Data |
| SCL (SCLK) | GPIO 12 | Serial Clock |
| MISO | GPIO 13 | Master In Slave Out (optional) |
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
| Button 1 | GPIO 1 | Context actions / Brightness |
| Button 2 | GPIO 2 | Recording (hold-to-record) |
| Button 3 | GPIO 3 | Screen switching |

**Wiring**: Connect one side to GPIO, other side to GND. Internal pull-ups enabled.

## Audio Configuration

### I2S Bus Sharing
The system dynamically switches the I2S bus between:
- **TX Mode**: Audio output (44.1kHz, 16-bit)
- **RX Mode**: Audio input (16kHz, 32-bit for voice)

