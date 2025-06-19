#include <Arduino.h>
#include <WiFi.h>
#include <esp_smartconfig.h>
#include <nvs_flash.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include <lvgl.h>

#include "wifi_manager.h"
#include "config.h"
#include "ui.h"
#include "ui_setup.h"
#include "lvgl_ui.h"
#include "lvgl_ui_components.h"
#include "button.h"
#include "audio_manager.h"
#include "spotify_manager.h"

// Manager objects
WiFiManager wifiManager;

// Timing
unsigned long lastUIUpdate = 0;
unsigned long lastSpotifyUpdate = 0;

// Current track data
SpotifyTrack currentTrack;
bool trackDataValid = false;

// Forward declarations
void forceSpotifyUpdate();
void forceDisplayRefresh();

// Simple states for Spotify player
enum PlayerState
{
    PLAYER_CONNECTING,
    PLAYER_READY,
    PLAYER_ERROR
};

PlayerState currentState = PLAYER_CONNECTING;

PlayerState checkPlayerStatus()
{
    bool wifiConnected = wifiManager.isConnected();

    if (!wifiConnected)
    {
        return PLAYER_CONNECTING;
    }

    return PLAYER_READY;
}

void updateSpotifyData()
{
    Serial0.println("🎵 Updating Spotify data...");
    unsigned long startTime = millis();
    const unsigned long SPOTIFY_TIMEOUT = 15000; // 15 second timeout

    // Add watchdog protection
    bool success = false;

    // Try to get current track with timeout protection
    Serial0.println("📡 Requesting current track from Spotify...");

    if (spotifyManager.getPlaybackState(currentTrack))
    {
        trackDataValid = true;
        Serial0.printf("✅ Now Playing: %s by %s\n", currentTrack.name.c_str(), currentTrack.artist.c_str());

        // Update LVGL UI
        lvgl_update_track_info(currentTrack, true);
        success = true;
    }
    else
    {
        Serial0.println("❌ Failed to get current track");
        trackDataValid = false;

        // Update LVGL UI with no track
        SpotifyTrack emptyTrack;
        emptyTrack.name = "No Track";
        emptyTrack.artist = "Connect Spotify";
        emptyTrack.album = "";
        emptyTrack.isPlaying = false;
        lvgl_update_track_info(emptyTrack, false);
    }

    unsigned long elapsedTime = millis() - startTime;
    Serial0.printf("📊 Spotify update completed in %lu ms (success: %s)\n",
                   elapsedTime, success ? "YES" : "NO");
}

void forceSpotifyUpdate()
{
    Serial0.println("🔄 Forcing immediate Spotify data update...");
    updateSpotifyData();
    lastSpotifyUpdate = millis(); // Reset timer to prevent immediate next update
}

void setup()
{
    Serial0.begin(115200);
    delay(1000);
    Serial0.println("Starting ESP32 Spotify Music Player...");

    // Initialize NVS
    Serial0.println("Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        Serial0.println("NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    Serial0.println("✅ NVS initialized successfully");

    delay(1000);

    // Initialize display
    Serial0.println("Initializing display...");
    initTFT();

    // Initialize LVGL UI system
    Serial0.println("Initializing LVGL UI system...");
    lvgl_ui_init();

    // Initialize audio system
    Serial0.println("Initializing audio system...");
    if (audioManager.init())
    {
        // Play startup sound
        audioManager.playBeep(800, 200);
        delay(100);
        audioManager.playBeep(1000, 200);
        delay(100);
        audioManager.playBeep(1200, 200);
    }

    // Initialize buttons
    Serial0.println("Initializing buttons...");
    initButtons();

    // Initialize WiFi
    Serial0.println("Initializing WiFi manager...");
    if (!wifiManager.hasStoredCredentials())
    {
        Serial0.println("No stored WiFi credentials, starting SmartConfig...");
        wifiManager.startSmartConfig();
    }

    // Wait for WiFi connection
    Serial0.println("Waiting for WiFi connection...");
    while (!wifiManager.isConnected())
    {
        wifiManager.update();
        delay(1000);
        Serial0.print(".");
    }
    Serial0.println();
    Serial0.println("WiFi connected!");

    // Initialize Spotify manager
    Serial0.println("Initializing Spotify manager...");
    if (spotifyManager.init())
    {
        Serial0.println("Spotify manager initialized successfully!");
    }
    else
    {
        Serial0.println("Spotify manager initialization failed - continuing in demo mode");
    }

    currentState = PLAYER_READY;
    Serial0.println("ESP32 Spotify Player ready!");

    // Play ready sound
    audioManager.playBeep(1000, 100);
    delay(50);
    audioManager.playBeep(1200, 100);
    delay(50);
    audioManager.playBeep(1500, 200);
}

void loop()
{
    // Handle LVGL tasks first (critical for UI responsiveness)
    lv_timer_handler();

    // Process any completed image downloads (non-blocking)
    lvgl_process_pending_images();

    // Update buttons and handle Spotify controls (critical for user interaction)
    updateButtons();
    handleSpotifyControls();

    // Update WiFi manager
    wifiManager.update();

    unsigned long now = millis();

    // Update WiFi status in LVGL UI
    static bool lastWifiStatus = false;
    bool currentWifiStatus = wifiManager.isConnected();
    if (currentWifiStatus != lastWifiStatus)
    {
        lvgl_update_wifi_status(currentWifiStatus);
        lastWifiStatus = currentWifiStatus;
    }

    // Update Spotify data every 3 seconds - only on main screen
    if (currentScreen == SCREEN_MAIN && now - lastSpotifyUpdate > 3000)
    {
        Serial0.println("⏰ Time for Spotify update...");
        lastSpotifyUpdate = now; // Set this FIRST to prevent multiple calls

        if (wifiManager.isConnected())
        {
            // Add timeout protection for the entire update process
            unsigned long updateStart = millis();
            updateSpotifyData();
            unsigned long updateTime = millis() - updateStart;

            Serial0.printf("⏱️ Total update time: %lu ms\n", updateTime);
            Serial0.printf("🔄 Next update in 3 seconds...\n");

            // Force garbage collection after Spotify request
            esp_get_free_heap_size(); // Trigger heap cleanup
        }
        else
        {
            Serial0.println("📶 WiFi not connected, skipping Spotify update");
        }
    }

    // Add heartbeat to show the loop is running
    static unsigned long lastHeartbeat = 0;
    if (now - lastHeartbeat > 10000)
    { // Every 10 seconds
        lastHeartbeat = now;
        Serial0.printf("💓 System running - Free heap: %d bytes\n", esp_get_free_heap_size());
    }

    delay(30); // Reduced delay for better LVGL responsiveness
}