#include "wifi_manager.h"

// ESP32-S3 DevKit C1 RGB LED pins
#define RGB_LED_R 38
#define RGB_LED_G 39
#define RGB_LED_B 40

WiFiManager::WiFiManager()
{
    _connected = false;
    _smartConfigActive = false;
    _lastReconnectAttempt = 0;
    _smartConfigStartTime = 0;
    _smartConfigStatus = "Ready";
    _preferences.begin("wifi", false);
    
    // Initialize RGB LED pins
    pinMode(RGB_LED_R, OUTPUT);
    pinMode(RGB_LED_G, OUTPUT);
    pinMode(RGB_LED_B, OUTPUT);
    setLEDColor(0, 0, 0); // Turn off LED initially
}

// RGB LED control functions
void WiFiManager::setLEDColor(int r, int g, int b)
{
    digitalWrite(RGB_LED_R, r > 0 ? HIGH : LOW);
    digitalWrite(RGB_LED_G, g > 0 ? HIGH : LOW);
    digitalWrite(RGB_LED_B, b > 0 ? HIGH : LOW);
}

void WiFiManager::setLEDStatus(LEDStatus status)
{
    switch(status) {
        case LED_OFF:
            setLEDColor(0, 0, 0);
            break;
        case LED_CONNECTING:
            setLEDColor(0, 0, 255); // Blue - connecting
            break;
        case LED_SMARTCONFIG:
            setLEDColor(255, 255, 0); // Yellow - SmartConfig active
            break;
        case LED_CONNECTED:
            setLEDColor(0, 255, 0); // Green - connected
            break;
        case LED_ERROR:
            setLEDColor(255, 0, 0); // Red - error
            break;
    }
}

bool WiFiManager::connect()
{
    setLEDStatus(LED_CONNECTING);
    
    // Only try stored credentials, no hardcoded fallback
    if (hasStoredCredentials() && connectWithStoredCredentials())
    {
        setLEDStatus(LED_CONNECTED);
        return true;
    }

    // If no stored credentials, start SmartConfig automatically
    Serial0.println("No stored WiFi credentials found. Starting SmartConfig...");
    startSmartConfig();
    return false;
}

bool WiFiManager::connectWithFallbackCredentials()
{
    Serial0.println("Attempting fallback WiFi connection...");
    _smartConfigStatus = "Connecting to fallback WiFi...";
    setLEDStatus(LED_CONNECTING);
    
    WiFi.begin("Thean Room 2.4", "Thean016");
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) // 15 second timeout
    {
        delay(500);
        Serial0.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        _connected = true;
        _smartConfigStatus = "Connected to fallback WiFi";
        setLEDStatus(LED_CONNECTED);
        Serial0.println("");
        Serial0.println("WiFi connected with fallback credentials");
        Serial0.print("IP address: ");
        Serial0.println(WiFi.localIP());
        
        // Save fallback credentials for future use
        saveCredentials("Thean Room 2.4", "Thean016");
        return true;
    }
    else
    {
        Serial0.println("");
        Serial0.println("Failed to connect with fallback credentials");
        _smartConfigStatus = "Fallback connection failed";
        setLEDStatus(LED_ERROR);
        return false;
    }
}

bool WiFiManager::startSmartConfig()
{
    if (_smartConfigActive)
    {
        return false; // Already active
    }

    Serial0.println("Starting SmartConfig...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();

    _smartConfigActive = true;
    _smartConfigStartTime = millis();
    _smartConfigStatus = "Waiting for ESP Touch app...";
    setLEDStatus(LED_SMARTCONFIG);

    Serial0.println("SmartConfig started. Use ESP Touch app to configure WiFi.");
    Serial0.println("Download 'ESP Touch' or 'ESP32 SmartConfig' app on your phone.");
    Serial0.println("Will fallback to default WiFi after 20 seconds...");
    return true;
}

void WiFiManager::stopSmartConfig()
{
    if (_smartConfigActive)
    {
        WiFi.stopSmartConfig();
        _smartConfigActive = false;
        _smartConfigStatus = "SmartConfig stopped";
        Serial0.println("SmartConfig stopped");
    }
}

bool WiFiManager::isSmartConfigActive()
{
    return _smartConfigActive;
}

String WiFiManager::getSmartConfigStatus()
{
    if (_smartConfigActive)
    {
        unsigned long elapsed = millis() - _smartConfigStartTime;
        unsigned long remaining = (20000 - elapsed) / 1000; // Changed to 20 seconds
        if (remaining > 0) {
            return "Waiting... " + String(remaining) + "s left (then fallback)";
        } else {
            return "Switching to fallback WiFi...";
        }
    }
    return _smartConfigStatus;
}

bool WiFiManager::hasStoredCredentials()
{
    return _preferences.isKey("ssid") && _preferences.isKey("password");
}

void WiFiManager::clearStoredCredentials()
{
    _preferences.remove("ssid");
    _preferences.remove("password");
    Serial0.println("Stored WiFi credentials cleared");
    _smartConfigStatus = "Credentials cleared";
}

bool WiFiManager::connectWithStoredCredentials()
{
    String ssid, password;
    if (!loadCredentials(ssid, password))
    {
        return false;
    }

    Serial0.print("Connecting with stored credentials to: ");
    Serial0.println(ssid);
    _smartConfigStatus = "Connecting to " + ssid;
    setLEDStatus(LED_CONNECTING);

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) // 15 second timeout
    {
        delay(500);
        Serial0.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        _connected = true;
        _smartConfigStatus = "Connected to " + ssid;
        setLEDStatus(LED_CONNECTED);
        Serial0.println("");
        Serial0.println("WiFi connected with stored credentials");
        Serial0.print("IP address: ");
        Serial0.println(WiFi.localIP());
        return true;
    }
    else
    {
        Serial0.println("");
        Serial0.println("Failed to connect with stored credentials. Clearing them.");
        clearStoredCredentials();
        _smartConfigStatus = "Connection failed, starting SmartConfig";
        setLEDStatus(LED_ERROR);
        return false;
    }
}

void WiFiManager::disconnect()
{
    stopSmartConfig();
    WiFi.disconnect();
    _connected = false;
    _smartConfigStatus = "Disconnected";
    setLEDStatus(LED_OFF);
    Serial0.println("WiFi disconnected");
}

void WiFiManager::update()
{
    // Handle SmartConfig
    if (_smartConfigActive)
    {
        // Check for 20-second timeout to fallback
        if (millis() - _smartConfigStartTime > 20000) // Changed to 20 seconds
        {
            Serial0.println("SmartConfig timeout after 20 seconds. Trying fallback credentials...");
            stopSmartConfig();
            
            if (connectWithFallbackCredentials())
            {
                return; // Successfully connected with fallback
            }
            else
            {
                // If fallback fails, restart SmartConfig
                Serial0.println("Fallback failed. Restarting SmartConfig...");
                delay(1000);
                startSmartConfig();
                return;
            }
        }

        // Check if SmartConfig is done
        if (WiFi.smartConfigDone())
        {
            Serial0.println("SmartConfig received.");
            _smartConfigStatus = "Credentials received, connecting...";
            setLEDStatus(LED_CONNECTING);

            // Wait for WiFi to connect
            Serial0.println("Waiting for WiFi connection...");
            unsigned long startTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000)
            {
                delay(500);
                Serial0.print(".");
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial0.println("");
                Serial0.println("WiFi connected via SmartConfig");
                Serial0.print("SSID: ");
                Serial0.println(WiFi.SSID());
                Serial0.print("IP: ");
                Serial0.println(WiFi.localIP());

                // Save the new credentials
                saveCredentials(WiFi.SSID(), WiFi.psk());
                _connected = true;
                _smartConfigStatus = "Connected to " + WiFi.SSID();
                setLEDStatus(LED_CONNECTED);
            }
            else
            {
                Serial0.println("");
                Serial0.println("Failed to connect after SmartConfig. Trying fallback...");
                _smartConfigStatus = "SmartConfig failed, trying fallback...";
                setLEDStatus(LED_ERROR);
                
                if (!connectWithFallbackCredentials())
                {
                    delay(2000);
                    startSmartConfig(); // Restart SmartConfig if fallback also fails
                    return;
                }
            }

            stopSmartConfig();
            return;
        }
    }

    // Regular WiFi status check
    if (WiFi.status() != WL_CONNECTED)
    {
        _connected = false;
        setLEDStatus(LED_ERROR);

        // Try to reconnect if enough time has passed since last attempt
        unsigned long currentTime = millis();
        if (currentTime - _lastReconnectAttempt > _reconnectInterval)
        {
            _lastReconnectAttempt = currentTime;
            Serial0.println("WiFi disconnected, attempting to reconnect...");

            // Try stored credentials first, then fallback, then SmartConfig
            if (!connectWithStoredCredentials())
            {
                if (!connectWithFallbackCredentials())
                {
                    startSmartConfig();
                }
            }
        }
    }
    else
    {
        _connected = true;
        setLEDStatus(LED_CONNECTED);
    }
}

bool WiFiManager::isConnected()
{
    return _connected;
}

int WiFiManager::getRSSI()
{
    if (_connected)
    {
        return WiFi.RSSI();
    }
    return 0;
}

String WiFiManager::getIP()
{
    if (_connected)
    {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getMACAddress()
{
    return WiFi.macAddress();
}

String WiFiManager::getSSID()
{
    if (_connected)
    {
        return WiFi.SSID();
    }
    return "Not connected";
}

void WiFiManager::saveCredentials(String ssid, String password)
{
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
    Serial0.println("WiFi credentials saved to flash memory");
}

bool WiFiManager::loadCredentials(String &ssid, String &password)
{
    if (!hasStoredCredentials())
    {
        return false;
    }

    ssid = _preferences.getString("ssid", "");
    password = _preferences.getString("password", "");

    return !ssid.isEmpty() && !password.isEmpty();
}