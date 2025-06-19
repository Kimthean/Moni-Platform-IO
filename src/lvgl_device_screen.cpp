#include "lvgl_device_screen.h"
#include "lvgl_ui_components.h"
#include "spotify_manager.h"
#include "audio_manager.h"
#include <Arduino.h>

// Device screen objects
lv_obj_t *device_screen;
lv_obj_t *device_title_label;
lv_obj_t *device_list_container;
lv_obj_t *device_instruction_label;

// Device data
static SpotifyDevice currentDevices[10];
static int currentDeviceCount = 0;
static int selectedDeviceIndex = 0;

void lvgl_create_device_screen()
{
    // Create device screen with dark theme
    device_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(device_screen, lv_color_hex(0x0f0f0f), 0);

    // Title label
    device_title_label = lv_label_create(device_screen);
    lv_label_set_text(device_title_label, "📱 Select Device");
    lv_obj_set_style_text_color(device_title_label, lv_color_hex(0x1db954), 0);
    lv_obj_set_style_text_font(device_title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(device_title_label, LV_ALIGN_TOP_MID, 0, 10);

    // Device list container
    device_list_container = lv_obj_create(device_screen);
    lv_obj_set_size(device_list_container, 300, 160);
    lv_obj_align(device_list_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(device_list_container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(device_list_container, 1, 0);
    lv_obj_set_style_border_color(device_list_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(device_list_container, 8, 0);
    lv_obj_clear_flag(device_list_container, LV_OBJ_FLAG_SCROLLABLE);

    // Instructions label
    device_instruction_label = lv_label_create(device_screen);
    lv_label_set_text(device_instruction_label, "🎵 Next/Prev: Navigate  🎵 Play: Select  🎵 Back: Triple Press");
    lv_obj_set_style_text_color(device_instruction_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(device_instruction_label, &lv_font_montserrat_10, 0);
    lv_obj_align(device_instruction_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_label_set_long_mode(device_instruction_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(device_instruction_label, 300);

    Serial0.println("✅ Device screen created");
}

void lvgl_update_device_list(SpotifyDevice devices[], int deviceCount)
{
    // Store device data
    currentDeviceCount = min(deviceCount, 10);
    for (int i = 0; i < currentDeviceCount; i++)
    {
        currentDevices[i] = devices[i];
    }

    // Only reset selectedDeviceIndex if this is the first load or current index is invalid
    if (selectedDeviceIndex >= currentDeviceCount || selectedDeviceIndex < 0)
    {
        // Find currently active device for initial selection
        selectedDeviceIndex = 0;
        for (int i = 0; i < currentDeviceCount; i++)
        {
            if (currentDevices[i].isActive)
            {
                selectedDeviceIndex = i;
                break;
            }
        }
        Serial0.printf("🔄 Reset selectedDeviceIndex to %d\n", selectedDeviceIndex);
    }
    else
    {
        Serial0.printf("🔒 Preserving selectedDeviceIndex at %d\n", selectedDeviceIndex);
    }

    // Clear existing device labels
    lv_obj_clean(device_list_container);

    // Create device labels
    for (int i = 0; i < currentDeviceCount; i++)
    {
        lv_obj_t *device_label = lv_label_create(device_list_container);

        // Format device text
        String deviceText = "";
        if (i == selectedDeviceIndex)
        {
            deviceText += "► ";
        }
        else
        {
            deviceText += "  ";
        }

        deviceText += currentDevices[i].name;
        if (currentDevices[i].isActive)
        {
            deviceText += " ✓";
        }
        deviceText += " (" + currentDevices[i].type + ")";
        if (currentDevices[i].volumePercent > 0)
        {
            deviceText += " " + String(currentDevices[i].volumePercent) + "%";
        }

        lv_label_set_text(device_label, deviceText.c_str());

        // Style based on selection and active state
        if (i == selectedDeviceIndex)
        {
            lv_obj_set_style_text_color(device_label, lv_color_hex(0x1db954), 0);
            lv_obj_set_style_bg_color(device_label, lv_color_hex(0x2a2a2a), 0);
            lv_obj_set_style_bg_opa(device_label, LV_OPA_50, 0);
        }
        else if (currentDevices[i].isActive)
        {
            lv_obj_set_style_text_color(device_label, lv_color_hex(0x4fc3f7), 0);
        }
        else
        {
            lv_obj_set_style_text_color(device_label, lv_color_hex(0xcccccc), 0);
        }

        lv_obj_set_style_text_font(device_label, &lv_font_montserrat_12, 0);
        lv_obj_set_pos(device_label, 10, 10 + (i * 25));
        lv_obj_set_width(device_label, 280);
        lv_label_set_long_mode(device_label, LV_LABEL_LONG_DOT);
    }

    Serial0.printf("Updated device list with %d devices, selectedIndex=%d\n", currentDeviceCount, selectedDeviceIndex);
}

void lvgl_navigate_devices(int direction)
{
    if (currentDeviceCount == 0)
    {
        Serial0.println("❌ No devices to navigate");
        return;
    }

    Serial0.printf("🧭 Navigation: direction=%d, currentIndex=%d, deviceCount=%d\n",
                   direction, selectedDeviceIndex, currentDeviceCount);

    int oldIndex = selectedDeviceIndex;
    int newIndex = selectedDeviceIndex + direction;

    // Wrap around
    if (newIndex < 0)
    {
        newIndex = currentDeviceCount - 1;
        Serial0.printf("🔄 Wrapped to bottom: newIndex=%d\n", newIndex);
    }
    if (newIndex >= currentDeviceCount)
    {
        newIndex = 0;
        Serial0.printf("🔄 Wrapped to top: newIndex=%d\n", newIndex);
    }

    selectedDeviceIndex = newIndex;
    Serial0.printf("📱 Navigated from device %d to %d: '%s'\n",
                   oldIndex, newIndex, currentDevices[newIndex].name.c_str());

    // Play navigation sound
    audioManager.playBeep(600 + (direction > 0 ? 200 : -200), 80);

    // Refresh the display
    lvgl_update_device_list(currentDevices, currentDeviceCount);
}

void lvgl_show_device_screen()
{
    Serial0.println("Showing device screen");
    lv_scr_load(device_screen);

    // Refresh device list when showing screen
    SpotifyDevice devices[10];
    int deviceCount;
    if (spotifyManager.getDevices(devices, 10, deviceCount))
    {
        lvgl_update_device_list(devices, deviceCount);
    }
    else
    {
        Serial0.println("Failed to get devices");
    }
}

void lvgl_hide_device_screen()
{
    Serial0.println("Hiding device screen");
    lv_scr_load(main_screen);
}

void lvgl_show_main_screen()
{
    lvgl_hide_device_screen();
}

void lvgl_select_current_device()
{
    if (currentDeviceCount == 0 || selectedDeviceIndex >= currentDeviceCount)
    {
        Serial0.println("❌ No devices available or invalid selection");
        audioManager.playBeep(400, 200);
        return;
    }

    Serial0.printf("📱 Attempting to select device %d: '%s'\n", selectedDeviceIndex, currentDevices[selectedDeviceIndex].name.c_str());
    Serial0.printf("   Device ID: '%s'\n", currentDevices[selectedDeviceIndex].id.c_str());
    Serial0.printf("   Currently active: %s\n", currentDevices[selectedDeviceIndex].isActive ? "YES" : "NO");

    // Don't switch if already active
    if (currentDevices[selectedDeviceIndex].isActive)
    {
        audioManager.playBeep(800, 200);
        Serial0.println("⚠️ Device already active - no transfer needed");
        return;
    }

    // Play selection sound
    audioManager.playBeep(1000, 150);
    delay(100);
    audioManager.playBeep(1200, 150);

    Serial0.printf("🔄 Transferring playback to device: %s\n", currentDevices[selectedDeviceIndex].name.c_str());

    // Transfer playback to selected device
    bool transferSuccess = spotifyManager.transferPlayback(currentDevices[selectedDeviceIndex].id);

    if (transferSuccess)
    {
        Serial0.println("✅ Playback transfer API call successful");

        // Mark the selected device as active and others as inactive
        for (int i = 0; i < currentDeviceCount; i++)
        {
            currentDevices[i].isActive = (i == selectedDeviceIndex);
        }

        // Update display immediately
        lvgl_update_device_list(currentDevices, currentDeviceCount);

        // Play success sound
        delay(300);
        audioManager.playBeep(1500, 200);

        Serial0.println("✅ Device selection completed successfully");

        // Auto-return to main screen after selection
        delay(1000);
        Serial0.println("🔙 Returning to main screen...");
        lvgl_show_main_screen();
    }
    else
    {
        Serial0.println("❌ Failed to transfer playback - API call failed");
        // Play error sound - distinctive double beep
        audioManager.playBeep(400, 200);
        delay(100);
        audioManager.playBeep(400, 200);
        delay(100);
        audioManager.playBeep(400, 200);
    }
}
