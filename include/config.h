#pragma once

// Display Configuration
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

// Display Pins for ESP32-S3
#define TFT_CS 15
#define TFT_DC 16
#define TFT_RST 17
#define TFT_MOSI 13 // SDA
#define TFT_SCLK 14 // SCL

// Colors (RGB565 format)
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0
#define COLOR_ORANGE 0xFD20
#define COLOR_GOLD 0xFEA0
#define COLOR_SILVER 0xC618
#define COLOR_GRAY 0x8410

// WiFi Configuration
// WIFI_SSID and WIFI_PASSWORD are now defined via build flags from environment variables
#define WIFI_TIMEOUT 15000 // 15 seconds for stored credential attempts

// Weather Configuration
// WEATHER_API_KEY is now defined via build flags from environment variables
#define WEATHER_UPDATE_INTERVAL 600000 // 10 minutes in milliseconds

// Time Configuration
#define NTP_SERVER "asia.pool.ntp.org"
#define NTP_SERVER_BACKUP "time.google.com"

// Timezone settings (multiply by 3600 to convert hours to seconds)
// Cambodia/Phnom Penh is GMT+7
#define GMT_OFFSET_SEC 3600 * 7 // Cambodia (Phnom Penh) is UTC+7
#define DAYLIGHT_OFFSET_SEC 0   // Cambodia doesn't use DST

// Time update interval
#define TIME_UPDATE_INTERVAL 1000 // 1 second in milliseconds

// UI Configuration
#define UI_UPDATE_INTERVAL 50 // 50ms for animations
#define UI_BACKGROUND COLOR_BLACK

// Spotify API Configuration
// SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, and SPOTIFY_REFRESH_TOKEN are now defined via build flags from environment variables