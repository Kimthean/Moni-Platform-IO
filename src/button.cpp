#include "button.h"
#include <Arduino.h>
#include "spotify_manager.h"
#include "audio_manager.h"
#include "lvgl_ui_components.h"
#include "lvgl_device_screen.h"

// Debounce settings
#define DEBOUNCE_DELAY 50 // milliseconds

// Button array
Button buttons[3] = {
    {BUTTON_1_PIN, false, false, 0, 0, false, false, 0, 0, 0, false, 0}, // Play/Pause (or Select in device screen)
    {BUTTON_2_PIN, false, false, 0, 0, false, false, 0, 0, 0, false, 0}, // Next Track (or Down in device screen)
    {BUTTON_3_PIN, false, false, 0, 0, false, false, 0, 0, 0, false, 0}  // Previous Track (or Up in device screen)
};

// Track if we've already processed a long press
static bool longPressProcessed[3] = {false, false, false};

// Current screen state
extern UIScreen currentScreen;

void initButtons()
{
    Serial0.println("Initializing Spotify control buttons...");

    // Initialize button pins as input with internal pullup
    for (int i = 0; i < 3; i++)
    {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        Serial0.printf("Button %d initialized on pin %d\n", i + 1, buttons[i].pin);
    }

    Serial0.println("✅ All buttons initialized with pullup resistors");
}

void updateButtons()
{
    unsigned long currentTime = millis();

    for (int i = 0; i < 3; i++)
    {
        // Read current button state (inverted because of pullup)
        bool reading = !digitalRead(buttons[i].pin);

        // Reset wasPressed flag
        buttons[i].wasPressed = false;

        // Debounce logic
        if (reading != buttons[i].lastState)
        {
            buttons[i].lastDebounceTime = currentTime;
        }

        if ((currentTime - buttons[i].lastDebounceTime) > DEBOUNCE_DELAY)
        {
            if (reading != buttons[i].currentState)
            {
                buttons[i].currentState = reading;

                if (buttons[i].currentState) // Button pressed
                {
                    buttons[i].isPressed = true;
                    buttons[i].wasPressed = true;
                    buttons[i].pressTime = currentTime;

                    // Triple press detection
                    if (currentTime - buttons[i].lastPressTime > TRIPLE_PRESS_MIN_INTERVAL)
                    {
                        if (currentTime - buttons[i].firstPressTime <= TRIPLE_PRESS_WINDOW)
                        {
                            buttons[i].pressCount++;
                        }
                        else
                        {
                            buttons[i].pressCount = 1;
                            buttons[i].firstPressTime = currentTime;
                        }
                        buttons[i].lastPressTime = currentTime;

                        // Set up delayed single press handling
                        buttons[i].hasPendingSinglePress = true;
                        buttons[i].pendingSinglePressTime = currentTime;
                    }
                }
                else // Button released
                {
                    buttons[i].isPressed = false;
                }
            }
        }

        buttons[i].lastState = reading;
    }
}

void processPendingSinglePresses()
{
    unsigned long currentTime = millis();

    for (int i = 0; i < 3; i++)
    {
        if (buttons[i].hasPendingSinglePress)
        {
            // Check if triple press window has expired
            if ((currentTime - buttons[i].pendingSinglePressTime) > TRIPLE_PRESS_WINDOW)
            {
                // If we didn't get a triple press, process the single press
                if (buttons[i].pressCount < 3)
                {
                    Serial0.printf("🔘 Processing delayed single press for button %d\n", i + 1);

                    if (currentScreen == SCREEN_MAIN)
                    {
                        switch (i)
                        {
                        case 0: // Button 1 - Play/Pause
                            handlePlayPause();
                            break;
                        case 1: // Button 2 - Next Track
                            handleNextTrack();
                            break;
                        case 2: // Button 3 - Previous Track
                            handlePreviousTrack();
                            break;
                        }
                    }
                    else if (currentScreen == SCREEN_DEVICES)
                    {
                        switch (i)
                        {
                        case 0: // Button 1 - Select device
                            lvgl_select_current_device();
                            break;
                        case 1: // Button 2 - Down
                            lvgl_navigate_devices(1);
                            break;
                        case 2: // Button 3 - Up
                            lvgl_navigate_devices(-1);
                            break;
                        }
                    }
                }

                // Clear pending state
                buttons[i].hasPendingSinglePress = false;
                buttons[i].pressCount = 0;
                buttons[i].firstPressTime = 0;
            }
        }
    }
}

bool isButtonTriplePressed(uint8_t buttonIndex)
{
    if (buttonIndex >= 3)
        return false;

    // Check if we have 3 presses within the time window
    if (buttons[buttonIndex].pressCount >= 3)
    {
        unsigned long currentTime = millis();
        if (currentTime - buttons[buttonIndex].firstPressTime <= TRIPLE_PRESS_WINDOW)
        {
            // Clear pending single press since we got a triple press
            buttons[buttonIndex].hasPendingSinglePress = false;

            // Reset the counter to prevent multiple triggers
            buttons[buttonIndex].pressCount = 0;
            buttons[buttonIndex].firstPressTime = 0;
            return true;
        }
        else
        {
            // Reset if window expired
            buttons[buttonIndex].pressCount = 0;
            buttons[buttonIndex].firstPressTime = 0;
        }
    }
    return false;
}

bool isButtonLongPressed(uint8_t buttonIndex)
{
    // Keep this function for compatibility but not used for screen switching
    return false;
}

void handleSpotifyControls()
{
    // Handle button presses based on current screen
    if (currentScreen == SCREEN_MAIN)
    {
        handleMainScreenButtons();
    }
    else if (currentScreen == SCREEN_DEVICES)
    {
        handleDeviceScreenButtons();
    }

    // Process any pending single presses
    processPendingSinglePresses();
}

void handleMainScreenButtons()
{
    // Check for triple press on any button to enter device screen
    for (int i = 0; i < 3; i++)
    {
        if (isButtonTriplePressed(i))
        {
            Serial0.printf("Triple press detected on button %d - switching to device screen\n", i + 1);
            audioManager.playBeep(1000, 100);
            delay(50);
            audioManager.playBeep(1200, 100);
            delay(50);
            audioManager.playBeep(1400, 100);
            lvgl_switch_screen(SCREEN_DEVICES);
            return;
        }
    }

    // Single press handling is now done in processPendingSinglePresses()
}

void handleDeviceScreenButtons()
{
    // Check for triple press to return to main screen
    for (int i = 0; i < 3; i++)
    {
        if (isButtonTriplePressed(i))
        {
            Serial0.printf("Triple press detected on button %d - returning to main screen\n", i + 1);
            audioManager.playBeep(800, 100);
            delay(50);
            audioManager.playBeep(600, 100);
            delay(50);
            audioManager.playBeep(400, 100);
            lvgl_switch_screen(SCREEN_MAIN);
            return;
        }
    }

    // Single press handling is now done in processPendingSinglePresses()
}

void handlePlayPause()
{
    Serial0.println("=== PLAY/PAUSE BUTTON PRESSED ===");

    // Play audio feedback
    audioManager.playBeep(800, 150);

    // Get current track to check if playing
    SpotifyTrack currentTrack;
    if (spotifyManager.getPlaybackState(currentTrack))
    {
        if (currentTrack.isPlaying)
        {
            Serial0.println("Action: Pausing playback");
            spotifyManager.pause();
        }
        else
        {
            Serial0.println("Action: Starting playback");
            spotifyManager.play();
        }
    }
    else
    {
        Serial0.println("Action: Attempting to start playback");
        spotifyManager.play();
    }

    // Wait a moment for Spotify to process, then update display
    delay(500);
    extern void forceSpotifyUpdate();
    forceSpotifyUpdate();
}

void handleNextTrack()
{
    Serial0.println("=== NEXT TRACK BUTTON PRESSED ===");

    // Play audio feedback - two quick beeps
    audioManager.playBeep(1000, 100);
    delay(50);
    audioManager.playBeep(1200, 100);

    Serial0.println("Action: Skipping to next track");
    spotifyManager.next();

    // Wait for Spotify to process, then update display
    delay(1000); // Longer delay for track change
    extern void forceSpotifyUpdate();
    forceSpotifyUpdate();
}

void handlePreviousTrack()
{
    Serial0.println("=== PREVIOUS TRACK BUTTON PRESSED ===");

    // Play audio feedback - two quick low beeps
    audioManager.playBeep(600, 100);
    delay(50);
    audioManager.playBeep(500, 100);

    Serial0.println("Action: Going to previous track");
    spotifyManager.previous();

    // Wait for Spotify to process, then update display
    delay(1000); // Longer delay for track change
    extern void forceSpotifyUpdate();
    forceSpotifyUpdate();
}

// Utility functions
bool isButtonPressed(uint8_t buttonIndex)
{
    if (buttonIndex >= 3)
        return false;
    return buttons[buttonIndex].isPressed;
}

bool wasButtonJustPressed(uint8_t buttonIndex)
{
    if (buttonIndex >= 3)
        return false;
    return buttons[buttonIndex].wasPressed;
}

bool wasButtonJustReleased(uint8_t buttonIndex)
{
    if (buttonIndex >= 3)
        return false;
    return !buttons[buttonIndex].currentState && buttons[buttonIndex].lastState;
}
