#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <esp_smartconfig.h>
#include <Preferences.h>

class WiFiManager
{
public:
    enum LEDStatus {
        LED_OFF,
        LED_CONNECTING,
        LED_SMARTCONFIG,
        LED_CONNECTED,
        LED_ERROR
    };

    WiFiManager();
    bool connect();
    bool startSmartConfig();
    void stopSmartConfig();
    bool isSmartConfigActive();
    String getSmartConfigStatus();
    bool hasStoredCredentials();
    void clearStoredCredentials();
    void disconnect();
    void update();
    bool isConnected();
    int getRSSI();
    String getIP();
    String getMACAddress();
    String getSSID();
    
    // New methods
    bool connectWithFallbackCredentials();
    void setLEDColor(int r, int g, int b);
    void setLEDStatus(LEDStatus status);

private:
    bool _connected;
    bool _smartConfigActive;
    unsigned long _lastReconnectAttempt;
    unsigned long _smartConfigStartTime;
    String _smartConfigStatus;
    Preferences _preferences;
    
    static const unsigned long _reconnectInterval = 30000; // 30 seconds
    static const unsigned long _smartConfigTimeout = 20000; // Changed to 20 seconds
    
    bool connectWithStoredCredentials();
    void saveCredentials(String ssid, String password);
    bool loadCredentials(String &ssid, String &password);
};

#endif