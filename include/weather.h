#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Weather data structure
struct WeatherData
{
    float temperature;
    float humidity;
    String condition;
    int conditionCode;
    float windSpeed;
    String windDirection;
    float pressure;
    bool dataValid;
};

class WeatherManager
{
public:
    WeatherManager();

    void init();
    void update();

    // Returns current weather data
    WeatherData getWeatherData();

    // Weather icon helper methods
    uint8_t getWeatherIcon();
    String getWeatherIconName();

private:
    WeatherData _weatherData;
    unsigned long _lastUpdate;
    String _city;

    bool fetchWeatherData();
    void parseWeatherData(String json);
    String getWindDirection(float degrees);
};