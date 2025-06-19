#ifndef UI_H
#define UI_H

#include <TFT_eSPI.h>
#include "time_manager.h"
#include "weather.h"
#include "wifi_manager.h"

// System state enum
enum SystemState
{
  STATE_CONNECTING,
  STATE_NORMAL,
  STATE_ERROR
};

// Screen modes enum
enum ScreenMode
{
  SCREEN_WEATHER,
  SCREEN_AUDIO_VIZ
};

// UI functions
void initDisplay();
void updateDisplay(TimeData timeData, WeatherData weatherData, bool wifiConnected, SystemState currentState);
void updateAudioVisualization(float *audioLevels, int numBands, bool isRecording);
void initAudioVisualizationDisplay(); // Add this line
void clearDisplay();
void switchToScreen(ScreenMode mode);
ScreenMode getCurrentScreen();

#endif