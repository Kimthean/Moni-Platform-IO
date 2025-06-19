const express = require('express');
const axios = require('axios');
const querystring = require('querystring');
require('dotenv').config();

const app = express();
const port = process.env.PORT || 3000;

// Spotify App Configuration
// These values are now loaded from environment variables (.env file)
const CLIENT_ID = process.env.CLIENT_ID;
const CLIENT_SECRET = process.env.CLIENT_SECRET;
const REDIRECT_URI = process.env.REDIRECT_URI || 'http://localhost:3000/callback';

// Check required environment variables
if (!CLIENT_ID || !CLIENT_SECRET) {
    console.error('❌ Missing required environment variables!');
    console.error('Please create a .env file with:');
    console.error('CLIENT_ID=your_spotify_client_id');
    console.error('CLIENT_SECRET=your_spotify_client_secret');
    console.error('REDIRECT_URI=your_redirect_uri (optional)');
    process.exit(1);
}

// Scopes needed for the ESP32 Spotify controller
const SCOPES = [
    'user-read-playback-state',
    'user-modify-playback-state',
    'user-read-currently-playing',
    'user-read-playback-position'
].join(' ');

// Store tokens temporarily (in production, use a proper database)
let tokens = {
    access_token: '',
    refresh_token: '',
    expires_in: 0,
    token_type: ''
};

app.use(express.static('public'));
app.use(express.json());

// Root route - Show login page
app.get('/', (req, res) => {
    res.send(`
        <!DOCTYPE html>
        <html>
        <head>
            <title>ESP32 Spotify Auth</title>
            <style>
                body { 
                    font-family: Arial, sans-serif; 
                    max-width: 800px; 
                    margin: 50px auto; 
                    padding: 20px;
                    background-color: #191414;
                    color: #1DB954;
                }
                .container { 
                    text-align: center; 
                    background: #282828;
                    padding: 40px;
                    border-radius: 10px;
                    box-shadow: 0 4px 8px rgba(0,0,0,0.3);
                }
                .btn { 
                    background-color: #1DB954; 
                    color: white; 
                    padding: 15px 30px; 
                    border: none; 
                    border-radius: 25px; 
                    font-size: 16px; 
                    cursor: pointer; 
                    text-decoration: none;
                    display: inline-block;
                    margin: 10px;
                    transition: background-color 0.3s;
                }
                .btn:hover { 
                    background-color: #1ed760; 
                }
                .warning {
                    background-color: #ff6b35;
                    color: white;
                    padding: 10px;
                    border-radius: 5px;
                    margin: 20px 0;
                }
                .code-block {
                    background-color: #000;
                    color: #1DB954;
                    padding: 20px;
                    border-radius: 5px;
                    font-family: monospace;
                    text-align: left;
                    margin: 20px 0;
                    overflow-x: auto;
                }
                h1 { color: #1DB954; }
                h2 { color: #1DB954; }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>🎵 ESP32 Spotify Authentication</h1>
                <p>This tool helps you get the OAuth tokens needed for your ESP32 Spotify controller.</p>
                
                <div class="warning">
                    <strong>⚠️ Before you start:</strong><br>
                    1. Create a .env file with your CLIENT_ID and CLIENT_SECRET<br>
                    2. Add "${REDIRECT_URI}" to your Spotify app's Redirect URIs<br>
                    3. Make sure your Spotify app has the required scopes enabled
                </div>

                <h2>📋 Setup Instructions:</h2>
                <div class="code-block">
1. Go to https://developer.spotify.com/dashboard
2. Create or edit your app: "ESP32 Spotify Player"
3. Add this redirect URI: ${REDIRECT_URI}
4. Copy your Client ID and Client Secret
5. Create a .env file in the spotify-auth-server folder:
   CLIENT_ID=your_client_id_here
   CLIENT_SECRET=your_client_secret_here
   REDIRECT_URI=${REDIRECT_URI}
6. Run: npm install
7. Restart this server
                </div>

                <div style="margin: 30px 0;">
                    <a href="/login" class="btn">🔐 Login with Spotify</a>
                    <a href="/tokens" class="btn">📄 View Current Tokens</a>
                </div>
                
                <p><small>Make sure your Spotify app is playing on a device before testing the ESP32 controller.</small></p>
            </div>
        </body>
        </html>
    `);
});

// Login route - Redirect to Spotify authorization
app.get('/login', (req, res) => {
    const state = Math.random().toString(36).substring(7);
    const authURL = 'https://accounts.spotify.com/authorize?' + querystring.stringify({
        response_type: 'code',
        client_id: CLIENT_ID,
        scope: SCOPES,
        redirect_uri: REDIRECT_URI,
        state: state
    });
    
    res.redirect(authURL);
});

// Callback route - Handle authorization code
app.get('/callback', async (req, res) => {
    const { code, state, error } = req.query;
    
    if (error) {
        return res.send(`
            <h1>❌ Authorization Error</h1>
            <p>Error: ${error}</p>
            <a href="/">← Back to Home</a>
        `);
    }
    
    if (!code) {
        return res.send(`
            <h1>❌ No Authorization Code</h1>
            <p>No authorization code received from Spotify.</p>
            <a href="/">← Back to Home</a>
        `);
    }

    try {
        // Exchange authorization code for tokens
        const tokenResponse = await axios.post('https://accounts.spotify.com/api/token', 
            querystring.stringify({
                grant_type: 'authorization_code',
                code: code,
                redirect_uri: REDIRECT_URI,
                client_id: CLIENT_ID,
                client_secret: CLIENT_SECRET
            }),
            {
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded'
                }
            }
        );

        tokens = tokenResponse.data;
        console.log('✅ Tokens obtained successfully!');
        console.log('Access Token:', tokens.access_token.substring(0, 20) + '...');
        console.log('Refresh Token:', tokens.refresh_token.substring(0, 20) + '...');

        res.redirect('/success');
    } catch (error) {
        console.error('❌ Error exchanging code for tokens:', error.response?.data || error.message);
        res.send(`
            <h1>❌ Token Exchange Error</h1>
            <p>Error: ${error.response?.data?.error_description || error.message}</p>
            <a href="/">← Back to Home</a>
        `);
    }
});

// Success route - Show tokens
app.get('/success', (req, res) => {
    if (!tokens.access_token) {
        return res.redirect('/');
    }

    res.send(`
        <!DOCTYPE html>
        <html>
        <head>
            <title>Tokens Retrieved!</title>
            <style>
                body { 
                    font-family: Arial, sans-serif; 
                    max-width: 1000px; 
                    margin: 20px auto; 
                    padding: 20px;
                    background-color: #191414;
                    color: #1DB954;
                }
                .container { 
                    background: #282828;
                    padding: 30px;
                    border-radius: 10px;
                    box-shadow: 0 4px 8px rgba(0,0,0,0.3);
                }
                .token-box { 
                    background-color: #000; 
                    color: #1DB954; 
                    padding: 15px; 
                    border-radius: 5px; 
                    font-family: monospace; 
                    margin: 10px 0;
                    word-break: break-all;
                    border: 1px solid #1DB954;
                }
                .copy-btn { 
                    background-color: #1DB954; 
                    color: white; 
                    border: none; 
                    padding: 5px 10px; 
                    border-radius: 3px; 
                    cursor: pointer; 
                    margin-left: 10px;
                }
                .copy-btn:hover { 
                    background-color: #1ed760; 
                }
                .warning {
                    background-color: #ff6b35;
                    color: white;
                    padding: 15px;
                    border-radius: 5px;
                    margin: 20px 0;
                }
                h1 { color: #1DB954; }
                h2 { color: #1DB954; }
                .esp32-config {
                    background-color: #000;
                    color: #1DB954;
                    padding: 20px;
                    border-radius: 5px;
                    font-family: monospace;
                    margin: 20px 0;
                    border: 2px solid #1DB954;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>🎉 Success! Tokens Retrieved</h1>
                
                <div class="warning">
                    <strong>⚠️ Important:</strong> Copy these tokens immediately and update your ESP32 code. 
                    The access token expires in ${tokens.expires_in} seconds (~${Math.round(tokens.expires_in/60)} minutes).
                </div>

                <h2>📋 Your Spotify Tokens:</h2>
                
                <p><strong>Access Token:</strong></p>
                <div class="token-box">${tokens.access_token}</div>
                
                <p><strong>Refresh Token:</strong></p>
                <div class="token-box">${tokens.refresh_token}</div>
                
                <p><strong>Token Type:</strong> ${tokens.token_type}</p>
                <p><strong>Expires In:</strong> ${tokens.expires_in} seconds</p>

                <h2>🔧 ESP32 Configuration:</h2>
                <p>Copy this into your <code>include/config.h</code> or <code>include/spotify_manager.h</code>:</p>
                <div class="esp32-config">
// Spotify API Configuration
#define SPOTIFY_CLIENT_ID "${CLIENT_ID}"
#define SPOTIFY_CLIENT_SECRET "${CLIENT_SECRET}"
#define SPOTIFY_REFRESH_TOKEN "${tokens.refresh_token}"

// Initial access token (will be refreshed automatically)
#define SPOTIFY_INITIAL_ACCESS_TOKEN "${tokens.access_token}"
                </div>

                <div style="margin: 30px 0;">
                    <a href="/tokens" class="copy-btn">View JSON Format</a>
                    <a href="/" class="copy-btn">← Back to Home</a>
                </div>

                <p><small>💡 The refresh token doesn't expire, but the access token does. Your ESP32 will automatically refresh it when needed.</small></p>
            </div>
        </body>
        </html>
    `);
});

// API route to get tokens as JSON
app.get('/tokens', (req, res) => {
    if (!tokens.access_token) {
        return res.json({ error: 'No tokens available. Please authenticate first.' });
    }
    
    res.json({
        access_token: tokens.access_token,
        refresh_token: tokens.refresh_token,
        token_type: tokens.token_type,
        expires_in: tokens.expires_in,
        client_id: CLIENT_ID,
        client_secret: CLIENT_SECRET,
        esp32_config: {
            SPOTIFY_CLIENT_ID: CLIENT_ID,
            SPOTIFY_CLIENT_SECRET: CLIENT_SECRET,
            SPOTIFY_REFRESH_TOKEN: tokens.refresh_token
        }
    });
});

// Health check
app.get('/health', (req, res) => {
    res.json({ 
        status: 'ok', 
        configured: CLIENT_ID !== 'your_client_id_here',
        has_tokens: !!tokens.access_token
    });
});

app.listen(port, () => {
    console.log('🎵 Spotify ESP32 Auth Server Started!');
    console.log(`📱 Open: http://localhost:${port}`);
    console.log('');
    
    if (CLIENT_ID === 'your_client_id_here') {
        console.log('⚠️  WARNING: Please update CLIENT_ID and CLIENT_SECRET in server.js');
        console.log('   1. Go to https://developer.spotify.com/dashboard');
        console.log('   2. Create/edit your app');
        console.log('   3. Add redirect URI: http://localhost:3000/callback');
        console.log('   4. Copy Client ID and Secret to server.js');
        console.log('   5. Restart this server');
    } else {
        console.log('✅ Configuration looks good!');
        console.log(`   Client ID: ${CLIENT_ID.substring(0, 10)}...`);
        console.log(`   Redirect URI: ${REDIRECT_URI}`);
    }
    console.log('');
}); 