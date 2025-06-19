#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

// Button pin definitions
#define BUTTON_1_PIN 1 // Play/Pause
#define BUTTON_2_PIN 2 // Next Track
#define BUTTON_3_PIN 3 // Previous Track

// Triple press settings
#define TRIPLE_PRESS_WINDOW 800      // milliseconds window for triple press
#define TRIPLE_PRESS_MIN_INTERVAL 50 // minimum time between presses

// Button structure
struct Button
{
    uint8_t pin;
    bool currentState;
    bool lastState;
    unsigned long lastDebounceTime;
    unsigned long pressTime;
    bool isPressed;
    bool wasPressed;

    // Triple press detection
    unsigned long lastPressTime;
    int pressCount;
    unsigned long firstPressTime;

    // Delayed single press handling
    bool hasPendingSinglePress;
    unsigned long pendingSinglePressTime;
};

// Function declarations
void initButtons();
void updateButtons();
void handleSpotifyControls();
bool isButtonPressed(uint8_t buttonIndex);
bool wasButtonJustPressed(uint8_t buttonIndex);
bool wasButtonJustReleased(uint8_t buttonIndex);
bool isButtonLongPressed(uint8_t buttonIndex);
bool isButtonTriplePressed(uint8_t buttonIndex);
void processPendingSinglePresses();

// Screen-specific button handlers
void handleMainScreenButtons();
void handleDeviceScreenButtons();

// Spotify control functions
void handlePlayPause();
void handleNextTrack();
void handlePreviousTrack();

#endif