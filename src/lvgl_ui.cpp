#include "lvgl_ui.h"
#include "lvgl_device_screen.h"
#include <Arduino.h>

// Main LVGL UI initialization - coordinates all sub-modules
void lvgl_ui_init()
{
    Serial0.println("🎨 Initializing LVGL UI system...");

    // Initialize display system
    lvgl_init_display();

    // Create main UI components
    lvgl_create_ui();

    // Create device selection screen
    lvgl_create_device_screen();

    Serial0.println("✅ LVGL UI system initialized successfully");
}