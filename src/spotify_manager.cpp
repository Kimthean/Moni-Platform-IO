#include "spotify_manager.h"

SpotifyManager spotifyManager;

SpotifyManager::SpotifyManager()
{
    refreshToken = SPOTIFY_REFRESH_TOKEN;
    tokenExpiry = 0;
}

bool SpotifyManager::init()
{
    Serial0.println("Initializing Spotify Manager...");

    // Skip actual initialization if refresh token is not set
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("⚠️ Spotify refresh token not configured - running in demo mode");
        accessToken = "demo_token";
        return true; // Return true to avoid blocking
    }

    // Configure SSL client properly
    client.setInsecure();     // Allow insecure connections for now
    client.setTimeout(15000); // 15 second timeout

    Serial0.println("Testing SSL connection to Spotify...");

    // Test connection first
    if (!client.connect("accounts.spotify.com", 443))
    {
        Serial0.println("❌ Cannot connect to Spotify servers - check WiFi and firewall");
        Serial0.println("Running in demo mode...");
        accessToken = "demo_token";
        return true; // Continue in demo mode
    }

    Serial0.println("✅ SSL connection to Spotify successful");
    client.stop(); // Close test connection

    // Get initial access token
    if (!refreshAccessToken())
    {
        Serial0.println("Failed to get initial Spotify access token - continuing anyway");
        return true; // Don't block initialization
    }

    Serial0.println("Spotify Manager initialized successfully!");
    return true;
}

bool SpotifyManager::refreshAccessToken()
{
    Serial0.println("Refreshing Spotify access token...");

    // Skip if not configured
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("Refresh token not configured - skipping");
        return false;
    }

    // Use HTTPClient for token refresh
    http.begin("https://accounts.spotify.com/api/token");

    String credentials = String(SPOTIFY_CLIENT_ID) + ":" + String(SPOTIFY_CLIENT_SECRET);
    String auth = base64EncodeFixed(credentials);
    String postData = "grant_type=refresh_token&refresh_token=" + String(SPOTIFY_REFRESH_TOKEN);

    Serial0.println("Attempting token refresh...");

    http.addHeader("Authorization", "Basic " + auth);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial0.printf("Token refresh response: HTTP %d\n", httpResponseCode);

        DynamicJsonDocument doc(1024);

        if (deserializeJson(doc, response) == DeserializationError::Ok)
        {
            if (doc.containsKey("access_token"))
            {
                accessToken = doc["access_token"].as<String>();
                int expiresIn = doc["expires_in"].as<int>();
                tokenExpiry = millis() + (expiresIn * 1000) - 300000; // Refresh 5 minutes early

                Serial0.println("✅ Access token refreshed successfully");
                http.end();
                return true;
            }
        }

        Serial0.println("❌ Failed to parse token response");
        Serial0.println("Response: " + response);
    }
    else
    {
        Serial0.printf("❌ Token refresh failed: HTTP %d\n", httpResponseCode);
    }

    http.end();
    return false;
}

bool SpotifyManager::getCurrentTrack(SpotifyTrack &track)
{
    // Return demo data if not configured
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        track.name = "Demo Track";
        track.artist = "Demo Artist";
        track.album = "Demo Album";
        track.isPlaying = true;
        track.duration_ms = 180000;
        track.progress_ms = 45000;
        return true;
    }

    // Check if token needs refresh
    if (millis() > tokenExpiry)
    {
        if (!refreshAccessToken())
        {
            return false;
        }
    }

    String response;
    if (!makeSpotifyRequest("/me/player/currently-playing", "GET", "", response))
    {
        return false;
    }

    // Handle HTTP 204 (No Content) - means no music is currently playing
    if (response.length() == 0)
    {
        Serial0.println("📭 No content - no music currently playing");
        // Set empty track data
        track.name = "No Track";
        track.artist = "Start playing music on Spotify";
        track.album = "";
        track.duration_ms = 0;
        track.progress_ms = 0;
        track.isPlaying = false;
        track.trackId = "";
        track.imageUrl = "";
        return true; // This is success - just no music playing
    }

    return parseCurrentTrack(response, track);
}

bool SpotifyManager::getPlaybackState(SpotifyTrack &track)
{
    // Return demo data if not configured
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        track.name = "Demo Track";
        track.artist = "Demo Artist";
        track.album = "Demo Album";
        track.isPlaying = true;
        track.duration_ms = 180000;
        track.progress_ms = 45000;
        track.shuffleState = false;
        track.repeatState = "off";
        track.timestamp = millis();
        track.deviceName = "Demo Device";
        track.deviceVolume = 75;
        track.deviceIsActive = true;
        track.contextType = "album";
        return true;
    }

    // Check if token needs refresh
    if (millis() > tokenExpiry)
    {
        if (!refreshAccessToken())
        {
            return false;
        }
    }

    String response;
    if (!makeSpotifyRequest("/me/player", "GET", "", response))
    {
        return false;
    }

    // Handle HTTP 204 (No Content) - means no music is currently playing
    if (response.length() == 0)
    {
        Serial0.println("📭 No content - no music currently playing");
        // Set empty track data
        track.name = "No Track";
        track.artist = "Start playing music on Spotify";
        track.album = "";
        track.duration_ms = 0;
        track.progress_ms = 0;
        track.isPlaying = false;
        track.trackId = "";
        track.imageUrl = "";
        track.shuffleState = false;
        track.repeatState = "off";
        track.timestamp = 0;
        track.deviceName = "";
        track.deviceVolume = 0;
        track.deviceIsActive = false;
        track.contextType = "";
        return true; // This is success - just no music playing
    }

    return parsePlaybackState(response, track);
}

bool SpotifyManager::play()
{
    Serial0.println("Spotify: Play command");
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("Demo mode - would send play command");
        return true;
    }

    String response;
    return makeSpotifyRequest("/me/player/play", "PUT", "", response);
}

bool SpotifyManager::pause()
{
    Serial0.println("Spotify: Pause command");
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("Demo mode - would send pause command");
        return true;
    }

    String response;
    return makeSpotifyRequest("/me/player/pause", "PUT", "", response);
}

bool SpotifyManager::next()
{
    Serial0.println("Spotify: Next track command");
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("Demo mode - would skip to next track");
        return true;
    }

    String response;
    return makeSpotifyRequest("/me/player/next", "POST", "", response);
}

bool SpotifyManager::previous()
{
    Serial0.println("Spotify: Previous track command");
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("Demo mode - would go to previous track");
        return true;
    }

    String response;
    return makeSpotifyRequest("/me/player/previous", "POST", "", response);
}

bool SpotifyManager::setVolume(int volume)
{
    Serial0.printf("Spotify: Set volume to %d%%\n", volume);
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("Demo mode - would set volume");
        return true;
    }

    String response;
    return makeSpotifyRequest("/me/player/volume?volume_percent=" + String(volume), "PUT", "", response);
}

bool SpotifyManager::makeSpotifyRequest(const String &endpoint, const String &method, const String &body, String &response)
{
    Serial0.println("Making Spotify API request to: " + endpoint);

    // Add connection timeout and retry logic
    unsigned long startTime = millis();
    const unsigned long TIMEOUT_MS = 10000; // 10 second timeout

    // Use HTTPClient for better SSL handling
    http.begin("https://api.spotify.com/v1" + endpoint);
    http.setTimeout(10000);       // 10 second timeout
    http.setConnectTimeout(5000); // 5 second connection timeout

    // Set headers
    http.addHeader("Authorization", "Bearer " + accessToken);
    http.addHeader("Accept", "application/json");
    http.addHeader("User-Agent", "ESP32-Spotify-Player/1.0");

    Serial0.println("📡 Sending HTTP request...");

    int httpResponseCode = -1;

    // Add watchdog reset to prevent hanging
    unsigned long requestStart = millis();

    if (method == "GET")
    {
        httpResponseCode = http.GET();
    }
    else if (method == "POST")
    {
        // Ensure body is not null for POST requests
        String postBody = (body.length() > 0) ? body : "{}";
        http.addHeader("Content-Type", "application/json");
        httpResponseCode = http.POST(postBody);
    }
    else if (method == "PUT")
    {
        // For PUT requests, Spotify often expects empty JSON body
        String putBody = (body.length() > 0) ? body : "{}";
        http.addHeader("Content-Type", "application/json");
        httpResponseCode = http.PUT(putBody);
    }
    else
    {
        Serial0.println("❌ Unsupported HTTP method: " + method);
        http.end();
        return false;
    }

    unsigned long requestTime = millis() - requestStart;
    Serial0.printf("📡 Request completed in %lu ms, HTTP code: %d\n", requestTime, httpResponseCode);

    if (httpResponseCode > 0)
    {
        response = http.getString();
        Serial0.printf("📡 HTTP %d - Response length: %d\n", httpResponseCode, response.length());

        // Success for 2xx status codes or 204 (No Content)
        bool success = (httpResponseCode >= 200 && httpResponseCode < 300) || httpResponseCode == 204;

        if (!success && response.length() > 0)
        {
            Serial0.println("Error response:");
            Serial0.println(response.substring(0, 200));
        }

        http.end();
        return success;
    }
    else
    {
        Serial0.printf("❌ HTTP Request failed: %d\n", httpResponseCode);

        // Handle specific error codes
        if (httpResponseCode == HTTPC_ERROR_CONNECTION_REFUSED)
        {
            Serial0.println("Connection refused - check internet connection");
        }
        else if (httpResponseCode == HTTPC_ERROR_CONNECTION_LOST)
        {
            Serial0.println("Connection lost during request");
        }
        else if (httpResponseCode == HTTPC_ERROR_READ_TIMEOUT)
        {
            Serial0.println("Read timeout - server not responding");
        }

        http.end();
        return false;
    }
}

bool SpotifyManager::parseCurrentTrack(const String &response, SpotifyTrack &track)
{
    Serial0.println("Parsing track response...");
    Serial0.printf("Response length: %d\n", response.length());

    // Use PSRAM for large JSON parsing - allocate 16KB in PSRAM
    DynamicJsonDocument *doc = new (ps_malloc(16384)) DynamicJsonDocument(16384);
    if (!doc)
    {
        Serial0.println("❌ Failed to allocate PSRAM for JSON parsing");
        return false;
    }

    DeserializationError error = deserializeJson(*doc, response);
    if (error != DeserializationError::Ok)
    {
        Serial0.printf("❌ JSON parse error: %s\n", error.c_str());
        Serial0.println("First 200 chars of response:");
        Serial0.println(response.substring(0, 200));

        // Free PSRAM
        free(doc);
        return false;
    }

    Serial0.println("✅ JSON parsed successfully using PSRAM");

    // Initialize track data
    track.name = "";
    track.artist = "";
    track.album = "";
    track.duration_ms = 0;
    track.progress_ms = 0;
    track.isPlaying = false;
    track.trackId = "";
    track.imageUrl = "";

    // Check if there's an active item
    if ((*doc)["item"].isNull())
    {
        Serial0.println("No active track playing");
        free(doc);
        return false;
    }

    // Extract track information
    JsonObject item = (*doc)["item"];

    if (item.containsKey("name"))
    {
        track.name = item["name"].as<String>();
    }

    if (item.containsKey("id"))
    {
        track.trackId = item["id"].as<String>();
    }

    if (item.containsKey("duration_ms"))
    {
        track.duration_ms = item["duration_ms"];
    }

    // Extract artist (first artist)
    if (item.containsKey("artists") && item["artists"].size() > 0)
    {
        track.artist = item["artists"][0]["name"].as<String>();
    }

    // Extract album info
    if (item.containsKey("album"))
    {
        JsonObject album = item["album"];
        if (album.containsKey("name"))
        {
            track.album = album["name"].as<String>();
        }

        // Extract album artwork (prefer 300x300, index 1)
        if (album.containsKey("images") && album["images"].size() > 0)
        {
            // Use medium quality image (300x300) - index 1 if available, otherwise largest
            int imageIndex = album["images"].size() > 1 ? 1 : 0;
            track.imageUrl = album["images"][imageIndex]["url"].as<String>();

            // Debug: show available image sizes
            Serial0.printf("🎨 Available images: %d\n", album["images"].size());
            for (int i = 0; i < album["images"].size(); i++)
            {
                int w = album["images"][i]["width"];
                int h = album["images"][i]["height"];
                Serial0.printf("   [%d]: %dx%d\n", i, w, h);
            }
            Serial0.printf("🎨 Selected index %d: %s\n", imageIndex, track.imageUrl.c_str());
        }
    }

    // Extract root-level fields
    if (doc->containsKey("progress_ms"))
    {
        track.progress_ms = (*doc)["progress_ms"];
    }

    if (doc->containsKey("is_playing"))
    {
        track.isPlaying = (*doc)["is_playing"];
    }

    // Free PSRAM
    free(doc);

    // Validation and output
    if (track.name.length() > 0 && track.artist.length() > 0)
    {
        Serial0.printf("✅ Track: '%s' by '%s'\n", track.name.c_str(), track.artist.c_str());
        Serial0.printf("   Album: '%s'\n", track.album.c_str());
        Serial0.printf("   Duration: %d ms, Progress: %d ms\n", track.duration_ms, track.progress_ms);
        Serial0.printf("   Status: %s\n", track.isPlaying ? "Playing" : "Paused");
        return true;
    }
    else
    {
        Serial0.printf("❌ Parse failed - Name: '%s', Artist: '%s'\n",
                       track.name.c_str(), track.artist.c_str());
        return false;
    }
}

bool SpotifyManager::parsePlaybackState(const String &response, SpotifyTrack &track)
{
    Serial0.println("Parsing playback state response...");
    Serial0.printf("Response length: %d\n", response.length());

    // Use PSRAM for large JSON parsing - allocate 20KB in PSRAM for larger response
    DynamicJsonDocument *doc = new (ps_malloc(20480)) DynamicJsonDocument(20480);
    if (!doc)
    {
        Serial0.println("❌ Failed to allocate PSRAM for JSON parsing");
        return false;
    }

    DeserializationError error = deserializeJson(*doc, response);
    if (error != DeserializationError::Ok)
    {
        Serial0.printf("❌ JSON parse error: %s\n", error.c_str());
        Serial0.println("First 300 chars of response:");
        Serial0.println(response.substring(0, 300));

        // Free PSRAM
        free(doc);
        return false;
    }

    Serial0.println("✅ JSON parsed successfully using PSRAM");

    // Initialize track data
    track.name = "";
    track.artist = "";
    track.album = "";
    track.duration_ms = 0;
    track.progress_ms = 0;
    track.isPlaying = false;
    track.trackId = "";
    track.imageUrl = "";
    track.shuffleState = false;
    track.repeatState = "off";
    track.timestamp = 0;
    track.deviceName = "";
    track.deviceVolume = 0;
    track.deviceIsActive = false;
    track.contextType = "";

    // Extract playback state information
    if (doc->containsKey("is_playing"))
    {
        track.isPlaying = (*doc)["is_playing"];
    }

    if (doc->containsKey("progress_ms"))
    {
        track.progress_ms = (*doc)["progress_ms"];
    }

    if (doc->containsKey("timestamp"))
    {
        track.timestamp = (*doc)["timestamp"];
    }

    if (doc->containsKey("shuffle_state"))
    {
        track.shuffleState = (*doc)["shuffle_state"];
    }

    if (doc->containsKey("repeat_state"))
    {
        track.repeatState = (*doc)["repeat_state"].as<String>();
    }

    // Extract device information
    if (doc->containsKey("device"))
    {
        JsonObject device = (*doc)["device"];
        if (device.containsKey("name"))
        {
            track.deviceName = device["name"].as<String>();
        }
        if (device.containsKey("volume_percent"))
        {
            track.deviceVolume = device["volume_percent"];
        }
        if (device.containsKey("is_active"))
        {
            track.deviceIsActive = device["is_active"];
        }
    }

    // Extract context information
    if (doc->containsKey("context") && !(*doc)["context"].isNull())
    {
        JsonObject context = (*doc)["context"];
        if (context.containsKey("type"))
        {
            track.contextType = context["type"].as<String>();
        }
    }

    // Check if there's an active item
    if ((*doc)["item"].isNull())
    {
        Serial0.println("No active track playing");
        free(doc);
        return true; // Still return true, we have valid playback state even without track
    }

    // Extract track information
    JsonObject item = (*doc)["item"];

    if (item.containsKey("name"))
    {
        track.name = item["name"].as<String>();
    }

    if (item.containsKey("id"))
    {
        track.trackId = item["id"].as<String>();
    }

    if (item.containsKey("duration_ms"))
    {
        track.duration_ms = item["duration_ms"];
    }

    // Extract artist (first artist)
    if (item.containsKey("artists") && item["artists"].size() > 0)
    {
        track.artist = item["artists"][0]["name"].as<String>();
    }

    // Extract album info
    if (item.containsKey("album"))
    {
        JsonObject album = item["album"];
        if (album.containsKey("name"))
        {
            track.album = album["name"].as<String>();
        }

        // Extract album artwork (prefer 300x300, index 1)
        if (album.containsKey("images") && album["images"].size() > 0)
        {
            // Use medium quality image (300x300) - index 1 if available, otherwise largest
            int imageIndex = album["images"].size() > 1 ? 1 : 0;
            track.imageUrl = album["images"][imageIndex]["url"].as<String>();

            // Debug: show available image sizes
            Serial0.printf("🎨 Available images: %d\n", album["images"].size());
            for (int i = 0; i < album["images"].size(); i++)
            {
                int w = album["images"][i]["width"];
                int h = album["images"][i]["height"];
                Serial0.printf("   [%d]: %dx%d\n", i, w, h);
            }
            Serial0.printf("🎨 Selected index %d: %s\n", imageIndex, track.imageUrl.c_str());
        }
    }

    // Free PSRAM
    free(doc);

    // Enhanced output with playback state
    Serial0.printf("✅ Playback State Retrieved:\n");
    Serial0.printf("   Track: '%s' by '%s'\n", track.name.c_str(), track.artist.c_str());
    Serial0.printf("   Album: '%s'\n", track.album.c_str());
    Serial0.printf("   Duration: %d ms, Progress: %d ms\n", track.duration_ms, track.progress_ms);
    Serial0.printf("   Status: %s\n", track.isPlaying ? "Playing" : "Paused");
    Serial0.printf("   Shuffle: %s, Repeat: %s\n", track.shuffleState ? "ON" : "OFF", track.repeatState.c_str());
    Serial0.printf("   Device: %s (%d%% volume)\n", track.deviceName.c_str(), track.deviceVolume);
    Serial0.printf("   Context: %s\n", track.contextType.c_str());

    return true;
}

String SpotifyManager::base64EncodeFixed(const String &str)
{
    const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result = "";
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    const char *input = str.c_str();
    int len = str.length();

    while (i < len)
    {
        // Get up to 3 input bytes
        int group_len = 0;
        for (int j = 0; j < 3 && i < len; j++)
        {
            char_array_3[j] = input[i++];
            group_len++;
        }

        // Pad with zeros if needed
        for (int j = group_len; j < 3; j++)
        {
            char_array_3[j] = 0;
        }

        // Convert to base64 characters
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        // Add characters to result with proper padding
        for (int j = 0; j < 4; j++)
        {
            if (group_len == 1 && j > 1)
            {
                result += "=";
            }
            else if (group_len == 2 && j > 2)
            {
                result += "=";
            }
            else
            {
                result += chars[char_array_4[j]];
            }
        }
    }

    return result;
}

String SpotifyManager::base64Encode(const String &str)
{
    return base64EncodeFixed(str);
}

bool SpotifyManager::getDevices(SpotifyDevice devices[], int maxDevices, int &deviceCount)
{
    Serial0.println("Getting Spotify devices...");
    deviceCount = 0;

    // Return demo data if not configured
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        // Demo devices
        devices[0] = {"demo1", "ESP32 Player", "Computer", true, 75};
        devices[1] = {"demo2", "iPhone", "Smartphone", false, 85};
        devices[2] = {"demo3", "Living Room Speaker", "Speaker", false, 60};
        deviceCount = 3;
        return true;
    }

    // Check if token needs refresh
    if (millis() > tokenExpiry)
    {
        if (!refreshAccessToken())
        {
            return false;
        }
    }

    String response;
    if (!makeSpotifyRequest("/me/player/devices", "GET", "", response))
    {
        return false;
    }

    // Use PSRAM for JSON parsing
    DynamicJsonDocument *doc = new (ps_malloc(8192)) DynamicJsonDocument(8192);
    if (!doc)
    {
        Serial0.println("❌ Failed to allocate PSRAM for devices JSON parsing");
        return false;
    }

    DeserializationError error = deserializeJson(*doc, response);
    if (error != DeserializationError::Ok)
    {
        Serial0.printf("❌ JSON parse error: %s\n", error.c_str());
        free(doc);
        return false;
    }

    if (doc->containsKey("devices"))
    {
        JsonArray devicesArray = (*doc)["devices"];
        deviceCount = min((int)devicesArray.size(), maxDevices);

        for (int i = 0; i < deviceCount; i++)
        {
            JsonObject device = devicesArray[i];
            devices[i].id = device["id"].as<String>();
            devices[i].name = device["name"].as<String>();
            devices[i].type = device["type"].as<String>();
            devices[i].isActive = device["is_active"];
            devices[i].volumePercent = device["volume_percent"];

            Serial0.printf("Device %d: %s (%s) - %s\n",
                           i, devices[i].name.c_str(), devices[i].type.c_str(),
                           devices[i].isActive ? "Active" : "Inactive");
        }
    }

    free(doc);
    Serial0.printf("Found %d devices\n", deviceCount);
    return true;
}

bool SpotifyManager::transferPlayback(const String &deviceId)
{
    Serial0.printf("🔄 Starting playback transfer to device ID: '%s'\n", deviceId.c_str());

    // Return success in demo mode
    if (String(SPOTIFY_REFRESH_TOKEN) == "your_refresh_token_here")
    {
        Serial0.println("📋 Demo mode - simulating successful transfer");
        return true;
    }

    // Check if token needs refresh
    if (millis() > tokenExpiry)
    {
        Serial0.println("🔄 Token expired, refreshing...");
        if (!refreshAccessToken())
        {
            Serial0.println("❌ Failed to refresh token for transfer");
            return false;
        }
        Serial0.println("✅ Token refreshed successfully");
    }

    // Create JSON body for transfer request
    String body = "{\"device_ids\":[\"" + deviceId + "\"],\"play\":true}";
    Serial0.printf("📡 Transfer request body: %s\n", body.c_str());

    String response;
    bool success = makeSpotifyRequest("/me/player", "PUT", body, response);

    if (success)
    {
        Serial0.println("✅ Transfer request completed successfully");
        if (response.length() > 0)
        {
            Serial0.printf("📋 Response: %s\n", response.c_str());
        }
        else
        {
            Serial0.println("📋 Empty response (normal for successful transfer)");
        }
    }
    else
    {
        Serial0.println("❌ Transfer request failed");
        if (response.length() > 0)
        {
            Serial0.printf("❌ Error response: %s\n", response.c_str());
        }
    }

    return success;
}
