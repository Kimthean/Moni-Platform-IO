#include "ui_setup.h"
#include "config.h"
#include <Arduino.h>

// Display object - DEFINE it here (not extern)
TFT_eSPI tft = TFT_eSPI();

void initTFT()
{
    Serial0.println("Initializing TFT display...");
    
    tft.init();
    tft.setRotation(1); // Rotate to landscape mode (0=portrait, 1=landscape, 2=portrait flipped, 3=landscape flipped)
    tft.fillScreen(TFT_BLACK);
    
    Serial0.printf("Display initialized: %dx%d (landscape)\n", tft.width(), tft.height());
}