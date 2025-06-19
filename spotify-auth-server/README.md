# 🎵 ESP32 Spotify Authentication Server

A simple Node.js web server to handle Spotify OAuth authentication and obtain the tokens needed for your ESP32 Spotify music controller.

## 🚀 Quick Start

### 1. Prerequisites
- Node.js (v14 or higher)
- A Spotify Developer account
- Your ESP32 Spotify Player app created on Spotify Developer Dashboard

### 2. Setup Your Spotify App

1. Go to [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Create a new app or edit your existing "ESP32 Spotify Player" app
3. **Important:** Add this Redirect URI: `http://localhost:3000/callback`
4. Note down your **Client ID** and **Client Secret**

### 3. Install and Configure

```bash
# Clone or navigate to the spotify-auth-server directory
cd spotify-auth-server

# Install dependencies
npm install

# Create a .env file with your Spotify credentials
# Copy .env.example to .env and update with your values:
cp .env.example .env

# Edit .env and add your Spotify app credentials:
# CLIENT_ID=your_actual_client_id_here
# CLIENT_SECRET=your_actual_client_secret_here
# REDIRECT_URI=http://localhost:3000/callback

# Start the server
npm start
```

### 4. Get Your Tokens

1. Open http://localhost:3000 in your browser
2. Click "🔐 Login with Spotify"
3. Authorize your app on Spotify
4. Copy the tokens from the success page
5. Update your ESP32 code with the tokens

## 📋 ESP32 Integration

After getting your tokens, update your ESP32 project:

### Option 1: Update `include/config.h`
```cpp
// Add these to your config.h
#define SPOTIFY_CLIENT_ID "your_client_id"
#define SPOTIFY_CLIENT_SECRET "your_client_secret"  
#define SPOTIFY_REFRESH_TOKEN "your_refresh_token"
```

### Option 2: Update `include/spotify_manager.h`
```cpp
// Replace the placeholder values
#define SPOTIFY_CLIENT_ID "your_actual_client_id"
#define SPOTIFY_CLIENT_SECRET "your_actual_client_secret"
#define SPOTIFY_REFRESH_TOKEN "your_actual_refresh_token"
```

## 🔧 API Endpoints

- `GET /` - Main page with setup instructions
- `GET /login` - Initiates Spotify OAuth flow
- `GET /callback` - Handles OAuth callback
- `GET /success` - Shows retrieved tokens
- `GET /tokens` - Returns tokens as JSON
- `GET /health` - Health check endpoint

## 🛠️ Development

```bash
# Install with dev dependencies
npm install

# Run with auto-reload
npm run dev
```

## 🔐 Security Notes

- **This is for development only** - Never expose client secrets in production
- The server stores tokens in memory only (they're lost when server restarts)
- **Environment variables are now supported** - Your secrets are stored in `.env` (not committed to git)
- For production, use proper token storage and secure environment variable management
- Consider using HTTPS for production deployments

## 🎯 Required Spotify Scopes

The server requests these scopes for your ESP32 controller:
- `user-read-playback-state` - Read current playback info
- `user-modify-playback-state` - Control playback (play/pause/skip)
- `user-read-currently-playing` - Get current track details
- `user-read-playback-position` - Get playback position

## 🐛 Troubleshooting

### "Invalid redirect URI" error
- Make sure you added `http://localhost:3000/callback` to your Spotify app's Redirect URIs
- Check that the URI is exactly: `http://localhost:3000/callback` (no trailing slash)

### "Invalid client" error  
- Double-check your Client ID and Client Secret in your `.env` file
- Make sure there are no extra spaces or characters
- Ensure your `.env` file is in the correct directory (`spotify-auth-server/.env`)

### "Access denied" error
- Make sure you have a Spotify Premium account (required for playback control)
- Try authorizing the app again

### Tokens expired
- Access tokens expire after 1 hour
- Your ESP32 will automatically refresh them using the refresh token
- The refresh token doesn't expire (unless you revoke app access)

## 📱 Testing Your ESP32

1. Make sure Spotify is playing on a device (phone, computer, etc.)
2. Upload your updated ESP32 code with the tokens
3. Use the buttons to control playback:
   - Button 1: Play/Pause
   - Button 2: Next track (short press) / Volume up (long press)
   - Button 3: Previous track (short press) / Volume down (long press)

## 🎵 What's Next?

Once you have your tokens:
1. Update your ESP32 code with the credentials
2. Upload to your ESP32
3. Test the button controls
4. Optionally create a custom UI to display current track info

Enjoy your ESP32 Spotify controller! 🎧 