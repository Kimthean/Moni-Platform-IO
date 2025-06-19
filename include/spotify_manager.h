#ifndef SPOTIFY_MANAGER_H
#define SPOTIFY_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Spotify API Configuration
#define SPOTIFY_CLIENT_ID "11629778e4d44ed8a93c81e9aff8a1a8"
#define SPOTIFY_CLIENT_SECRET "9dd279bba23e4e82a64112a2593d558c"
#define SPOTIFY_REFRESH_TOKEN "AQDtx-Q7GzoAg9xXtTA57xa4r8bZ9zPu2Bg5NKzo3YHMmmWM8BSiA1IR4GT_Pl5CKqEfxEyMPitiqHH_SZvIFbf7bCnDfj9NcQvU24GrvcNqxTbKHKJ8Mr4Gq5dTT2sVWqc"

// Spotify API URLs
#define SPOTIFY_TOKEN_URL "https://accounts.spotify.com/api/token"
#define SPOTIFY_API_URL "https://api.spotify.com/v1"

struct SpotifyTrack
{
    String name;
    String artist;
    String album;
    String imageUrl;
    int duration_ms;
    int progress_ms;
    bool isPlaying;
    String trackId;
    
    // Enhanced playback state info
    bool shuffleState;
    String repeatState;  // "off", "track", "context"
    unsigned long timestamp;
    String deviceName;
    int deviceVolume;
    bool deviceIsActive;
    String contextType;  // "album", "playlist", "artist", etc.
};

struct SpotifyDevice
{
    String id;
    String name;
    String type;
    bool isActive;
    int volumePercent;
};

class SpotifyManager
{
public:
    SpotifyManager();
    bool init();
    bool refreshAccessToken();
    bool getCurrentTrack(SpotifyTrack &track);
    bool getPlaybackState(SpotifyTrack &track);
    bool play();
    bool pause();
    bool next();
    bool previous();
    bool setVolume(int volume); // 0-100
    bool getDevices(SpotifyDevice devices[], int maxDevices, int &deviceCount);
    bool transferPlayback(const String &deviceId);
    String getAccessToken() { return accessToken; }

private:
    String accessToken;
    String refreshToken;
    unsigned long tokenExpiry;
    WiFiClientSecure client;
    HTTPClient http;

    bool makeSpotifyRequest(const String &endpoint, const String &method, const String &body, String &response);
    bool parseCurrentTrack(const String &response, SpotifyTrack &track);
    bool parsePlaybackState(const String &response, SpotifyTrack &track);
    String base64Encode(const String &str);
    String base64EncodeFixed(const String &str);
};

extern SpotifyManager spotifyManager;

#endif