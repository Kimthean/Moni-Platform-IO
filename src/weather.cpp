#include "weather.h"
#include <cmath>  // For round() function
#include <WiFi.h> // For WiFi status checks
#include <HTTPClient.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager()
{
    _lastUpdate = 0;
    _city = "Phnom Penh"; // Default city

    // Initialize with default values
    _weatherData.temperature = 0.0;
    _weatherData.humidity = 0.0;
    _weatherData.condition = "Unknown";
    _weatherData.conditionCode = 0;
    _weatherData.windSpeed = 0.0;
    _weatherData.windDirection = "";
    _weatherData.pressure = 0.0;
    _weatherData.dataValid = false;
}

void WeatherManager::init()
{
    Serial0.print("Free heap before weather fetch: ");
    Serial0.println(ESP.getFreeHeap());
    // Initial data fetch
    fetchWeatherData();
    Serial0.print("Free heap after weather fetch: ");
    Serial0.println(ESP.getFreeHeap());
}

void WeatherManager::update()
{
    unsigned long currentMillis = millis();

    // Update weather data periodically
    if (currentMillis - _lastUpdate >= WEATHER_UPDATE_INTERVAL || _lastUpdate == 0)
    {
        fetchWeatherData();
    }
}

WeatherData WeatherManager::getWeatherData()
{
    return _weatherData;
}

bool WeatherManager::fetchWeatherData()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial0.println("Weather update failed: WiFi not connected");
        return false;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial0.print("WiFi RSSI: ");
        Serial0.println(WiFi.RSSI());
    }

    HTTPClient https;

    // URL encode the city name by replacing spaces with %20
    String encodedCity = _city;
    encodedCity.replace(" ", "%20");

    String url = "https://api.openweathermap.org/data/2.5/weather?q=" + encodedCity +
                 "&units=metric&appid=" + String(WEATHER_API_KEY);

    Serial0.print("Weather API URL: ");
    Serial0.println(url);

    // Configure HTTPClient for better reliability
    https.begin(url);
    https.setTimeout(10000); // 10 second timeout
    https.setReuse(false);   // Don't reuse connections

    int httpCode = https.GET();
    Serial0.print("HTTP Response Code: ");
    Serial0.println(httpCode);

    if (httpCode == 200)
    {
        // Use getString() instead of manual stream reading
        String payload = https.getString();
        https.end(); // Close connection immediately

        if (payload.length() > 0)
        {
            Serial0.println("Weather data received successfully");
            Serial0.print("Payload length: ");
            Serial0.println(payload.length());
            Serial0.print("Free heap before parsing: ");
            Serial0.println(ESP.getFreeHeap());

            parseWeatherData(payload);
            _lastUpdate = millis();

            Serial0.print("Free heap after parsing: ");
            Serial0.println(ESP.getFreeHeap());
            return true;
        }
        else
        {
            Serial0.println("Failed to read response payload");
            _weatherData.dataValid = false;
            return false;
        }
    }
    else
    {
        Serial0.print("Weather update failed with HTTP code: ");
        Serial0.println(httpCode);
        https.end();
        _weatherData.dataValid = false;
        return false;
    }
}

void WeatherManager::parseWeatherData(String json)
{
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        Serial0.println("JSON parsing failed");
        _weatherData.dataValid = false;
        return;
    }
    serializeJson(doc, Serial0);

    _weatherData.temperature = doc["main"]["temp"];
    _weatherData.humidity = doc["main"]["humidity"];
    _weatherData.pressure = doc["main"]["pressure"];
    _weatherData.condition = doc["weather"][0]["main"].as<String>();
    _weatherData.conditionCode = doc["weather"][0]["id"];
    _weatherData.windSpeed = doc["wind"]["speed"];

    float windDegrees = doc["wind"]["deg"];
    _weatherData.windDirection = getWindDirection(windDegrees);

    _weatherData.dataValid = true;
}

String WeatherManager::getWindDirection(float degrees)
{
    const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int index = static_cast<int>(round(degrees / 45.0)) % 8;
    return String(directions[index]);
}

uint8_t WeatherManager::getWeatherIcon()
{
    int code = _weatherData.conditionCode;

    if (code >= 200 && code < 300)
    {
        return 0; // Thunderstorm
    }
    else if (code >= 300 && code < 400)
    {
        return 1; // Drizzle
    }
    else if (code >= 500 && code < 600)
    {
        return 2; // Rain
    }
    else if (code >= 600 && code < 700)
    {
        return 3; // Snow
    }
    else if (code >= 700 && code < 800)
    {
        return 4; // Atmosphere (fog, mist)
    }
    else if (code == 800)
    {
        return 5; // Clear
    }
    else if (code > 800)
    {
        return 6; // Clouds
    }

    return 7; // Unknown
}

String WeatherManager::getWeatherIconName()
{
    int code = _weatherData.conditionCode;

    if (code >= 200 && code < 300)
    {
        return "Thunderstorm";
    }
    else if (code >= 300 && code < 400)
    {
        return "Drizzle";
    }
    else if (code >= 500 && code < 600)
    {
        return "Rain";
    }
    else if (code >= 600 && code < 700)
    {
        return "Snow";
    }
    else if (code >= 700 && code < 800)
    {
        return "Fog";
    }
    else if (code == 800)
    {
        return "Clear";
    }
    else if (code > 800)
    {
        return "Cloudy";
    }

    return "Unknown";
}