#include "ui.h"
#include "ui_setup.h"
#include "config.h"
#include <Arduino.h>
#include "audio_manager.h"

// Colors
#define COLOR_BG 0x1a1a2e
#define COLOR_WHITE 0xFFFF
#define COLOR_GREEN 0x07E0
#define COLOR_YELLOW 0xFFE0
#define COLOR_RED 0xF800
#define COLOR_BLUE 0x001F
#define COLOR_CYAN 0x07FF
#define COLOR_ORANGE 0xFD20

// Static variables to track previous values
static String lastTimeStr = "";
static String lastDateStr = "";
static String lastWeatherStr = "";
static float lastTemp = -999;
static float lastHumidity = -999;
static float lastWind = -999;
static bool lastWifiState = false;
static SystemState lastDisplayState = STATE_CONNECTING;

void initDisplay()
{
    Serial0.println("Initializing simple TFT display...");

    tft.fillScreen(TFT_BLACK);

    // Draw title
    tft.setTextColor(COLOR_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Weather Station", DISPLAY_WIDTH / 2, 10);

    // Draw static labels
    tft.setTextSize(1);
    tft.setTextColor(COLOR_YELLOW, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Time:", 10, 50);
    tft.drawString("Date:", 10, 70);
    tft.drawString("Weather:", 10, 100);
    tft.drawString("Temp:", 10, 120);
    tft.drawString("Humidity:", 10, 140);
    tft.drawString("Wind:", 10, 160);
    tft.drawString("WiFi:", 10, 190);
    tft.drawString("Status:", 10, 210);

    Serial0.println("Simple TFT display initialized");
}

void updateDisplay(TimeData timeData, WeatherData weatherData, bool wifiConnected, SystemState currentState)
{
    // Update time
    String timeStr = "--:--:--";
    if (timeData.timeValid)
    {
        timeStr = String(timeData.hour < 10 ? "0" : "") + String(timeData.hour) + ":" +
                  String(timeData.minute < 10 ? "0" : "") + String(timeData.minute) + ":" +
                  String(timeData.second < 10 ? "0" : "") + String(timeData.second);
    }

    if (timeStr != lastTimeStr)
    {
        tft.setTextColor(COLOR_GREEN, TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextDatum(TL_DATUM);
        tft.fillRect(80, 45, 200, 20, TFT_BLACK);
        tft.drawString(timeStr, 80, 50);
        lastTimeStr = timeStr;
    }

    // Update date
    String dateStr = "--/--/----";
    if (timeData.timeValid)
    {
        dateStr = String(timeData.day < 10 ? "0" : "") + String(timeData.day) + "/" +
                  String(timeData.month < 10 ? "0" : "") + String(timeData.month) + "/" +
                  String(timeData.year);
    }

    if (dateStr != lastDateStr)
    {
        tft.setTextColor(COLOR_WHITE, TFT_BLACK);
        tft.setTextSize(1);
        tft.fillRect(80, 70, 200, 15, TFT_BLACK);
        tft.drawString(dateStr, 80, 70);
        lastDateStr = dateStr;
    }

    // Update weather
    String weatherStr = weatherData.dataValid ? weatherData.condition : (currentState == STATE_CONNECTING ? "Loading..." : "No Data");

    if (weatherStr != lastWeatherStr)
    {
        tft.setTextColor(COLOR_YELLOW, TFT_BLACK);
        tft.setTextSize(1);
        tft.fillRect(80, 100, 200, 15, TFT_BLACK);
        tft.drawString(weatherStr, 80, 100);
        lastWeatherStr = weatherStr;
    }

    // Update temperature
    if (weatherData.dataValid && weatherData.temperature != lastTemp)
    {
        tft.setTextColor(COLOR_RED, TFT_BLACK);
        tft.fillRect(80, 120, 200, 15, TFT_BLACK);
        tft.drawString(String(weatherData.temperature, 1) + "°C", 80, 120);
        lastTemp = weatherData.temperature;
    }
    else if (!weatherData.dataValid && lastTemp != -999)
    {
        tft.setTextColor(COLOR_RED, TFT_BLACK);
        tft.fillRect(80, 120, 200, 15, TFT_BLACK);
        tft.drawString("--°C", 80, 120);
        lastTemp = -999;
    }

    // Update humidity
    if (weatherData.dataValid && weatherData.humidity != lastHumidity)
    {
        tft.setTextColor(COLOR_CYAN, TFT_BLACK);
        tft.fillRect(80, 140, 200, 15, TFT_BLACK);
        tft.drawString(String(weatherData.humidity, 0) + "% RH", 80, 140);
        lastHumidity = weatherData.humidity;
    }
    else if (!weatherData.dataValid && lastHumidity != -999)
    {
        tft.setTextColor(COLOR_CYAN, TFT_BLACK);
        tft.fillRect(80, 140, 200, 15, TFT_BLACK);
        tft.drawString("--% RH", 80, 140);
        lastHumidity = -999;
    }

    // Update wind speed
    if (weatherData.dataValid && weatherData.windSpeed != lastWind)
    {
        tft.setTextColor(COLOR_BLUE, TFT_BLACK);
        tft.fillRect(80, 160, 200, 15, TFT_BLACK);
        tft.drawString(String(weatherData.windSpeed, 1) + " m/s", 80, 160);
        lastWind = weatherData.windSpeed;
    }
    else if (!weatherData.dataValid && lastWind != -999)
    {
        tft.setTextColor(COLOR_BLUE, TFT_BLACK);
        tft.fillRect(80, 160, 200, 15, TFT_BLACK);
        tft.drawString("-- m/s", 80, 160);
        lastWind = -999;
    }

    // Update WiFi status
    if (wifiConnected != lastWifiState)
    {
        uint16_t color = wifiConnected ? COLOR_GREEN : COLOR_RED;
        String status = wifiConnected ? "Connected" : "Disconnected";
        tft.setTextColor(color, TFT_BLACK);
        tft.fillRect(80, 190, 200, 15, TFT_BLACK);
        tft.drawString(status, 80, 190);
        lastWifiState = wifiConnected;
    }

    // Update system status
    if (currentState != lastDisplayState)
    {
        uint16_t color;
        String status;

        switch (currentState)
        {
        case STATE_CONNECTING:
            color = COLOR_ORANGE;
            status = "Connecting";
            break;
        case STATE_NORMAL:
            color = COLOR_GREEN;
            status = "Normal";
            break;
        case STATE_ERROR:
            color = COLOR_RED;
            status = "Error";
            break;
        }

        tft.setTextColor(color, TFT_BLACK);
        tft.fillRect(80, 210, 200, 15, TFT_BLACK);
        tft.drawString(status, 80, 210);
        lastDisplayState = currentState;
    }
}

void clearDisplay()
{
    tft.fillScreen(TFT_BLACK);
    // Reset tracking variables
    lastTimeStr = "";
    lastDateStr = "";
    lastWeatherStr = "";
    lastTemp = -999;
    lastHumidity = -999;
    lastWind = -999;
    lastWifiState = false;
    lastDisplayState = STATE_CONNECTING;
}

// Add screen management variables
static ScreenMode currentScreenMode = SCREEN_WEATHER;
static float lastAudioLevels[NUM_BANDS] = {0};
static bool lastRecordingState = false;

void switchToScreen(ScreenMode mode)
{
    currentScreenMode = mode;
    clearDisplay();

    if (mode == SCREEN_WEATHER)
    {
        initDisplay(); // Reinitialize weather display
    }
    else if (mode == SCREEN_AUDIO_VIZ)
    {
        initAudioVisualizationDisplay();
    }
}

ScreenMode getCurrentScreen()
{
    return currentScreenMode;
}

void initAudioVisualizationDisplay()
{
    Serial0.println("Initializing audio visualization display...");

    tft.fillScreen(TFT_BLACK);

    // Draw title
    tft.setTextColor(COLOR_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Audio Visualizer", DISPLAY_WIDTH / 2, 10);

    // Draw static labels
    tft.setTextSize(1);
    tft.setTextColor(COLOR_YELLOW, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Level:", 10, 50);
    tft.drawString("Frequency Bands:", 10, 80);
    tft.drawString("Recording:", 10, 280);

    // Draw frequency band labels
    const char *bandLabels[] = {"63Hz", "125Hz", "250Hz", "500Hz", "1kHz", "2kHz", "4kHz", "8kHz"};
    for (int i = 0; i < NUM_BANDS; i++)
    {
        int x = 10 + (i * 28);
        tft.drawString(bandLabels[i], x, 100);
    }

    // Draw control instructions
    tft.setTextColor(COLOR_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("BTN1: Back to Weather", 10, 300);
    tft.drawString("BTN2: Start/Stop Recording", 10, 310);
}

void updateAudioVisualization(float *audioLevels, int numBands, bool isRecording)
{
    if (currentScreenMode != SCREEN_AUDIO_VIZ)
        return;

    // Update audio level meter
    float currentLevel = audioManager.getAudioLevel();
    int levelBarWidth = (int)(currentLevel * 200); // Scale to 200 pixels

    // Clear previous level bar
    tft.fillRect(60, 50, 200, 15, TFT_BLACK);

    // Draw new level bar with color coding
    uint16_t levelColor = COLOR_GREEN;
    if (currentLevel > 0.7)
        levelColor = COLOR_RED;
    else if (currentLevel > 0.4)
        levelColor = COLOR_YELLOW;

    tft.fillRect(60, 50, levelBarWidth, 15, levelColor);
    tft.drawRect(60, 50, 200, 15, COLOR_WHITE); // Border

    // Update frequency bands visualization
    for (int i = 0; i < numBands && i < NUM_BANDS; i++)
    {
        int x = 10 + (i * 28);
        int barHeight = (int)(audioLevels[i] * 100); // Scale to 100 pixels

        // Clear previous bar
        tft.fillRect(x, 115, 25, 100, TFT_BLACK);

        // Draw new bar
        uint16_t barColor = COLOR_BLUE;
        if (audioLevels[i] > 0.7)
            barColor = COLOR_RED;
        else if (audioLevels[i] > 0.4)
            barColor = COLOR_ORANGE;
        else if (audioLevels[i] > 0.2)
            barColor = COLOR_YELLOW;

        tft.fillRect(x, 215 - barHeight, 25, barHeight, barColor);
        tft.drawRect(x, 115, 25, 100, COLOR_WHITE); // Border

        lastAudioLevels[i] = audioLevels[i];
    }

    // Update recording status
    if (isRecording != lastRecordingState)
    {
        tft.fillRect(80, 280, 100, 10, TFT_BLACK); // Clear previous status
        tft.setTextColor(isRecording ? COLOR_RED : COLOR_GREEN, TFT_BLACK);
        tft.drawString(isRecording ? "RECORDING" : "STANDBY", 80, 280);
        lastRecordingState = isRecording;
    }
}