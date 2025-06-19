#include "lvgl_ui_components.h"
#include "lvgl_album_art.h"
#include "lvgl_device_screen.h"
#include <Arduino.h>

// Current screen state
UIScreen currentScreen = SCREEN_MAIN;

// LVGL UI objects
lv_obj_t *main_screen;
lv_obj_t *album_img;
lv_obj_t *track_label;
lv_obj_t *artist_label;
lv_obj_t *album_label;
lv_obj_t *status_label;
lv_obj_t *wifi_status_label;

// Create the modern UI
void lvgl_create_ui()
{
    // Create main screen with dark theme
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x0f0f0f), 0);
    lv_scr_load(main_screen);

    // Spotify icon (top right)
    lv_obj_t *spotify_icon = lv_label_create(main_screen);
    lv_label_set_text(spotify_icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(spotify_icon, lv_color_hex(0x1db954), 0);
    lv_obj_set_style_text_font(spotify_icon, &lv_font_montserrat_16, 0);
    lv_obj_align(spotify_icon, LV_ALIGN_TOP_RIGHT, -10, 5);

    // Title label
    lv_obj_t *title_label = lv_label_create(main_screen);
    lv_label_set_text(title_label, "Spotify Player");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x1db954), 0); // Spotify green
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);

    // WiFi status icon (top left)
    wifi_status_label = lv_label_create(main_screen);
    lv_label_set_text(wifi_status_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_16, 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_LEFT, 10, 5);

    // Album artwork container (170x170 with rounded corners)
    static lv_obj_t *album_container = lv_obj_create(main_screen);
    lv_obj_set_size(album_container, 170, 170);
    lv_obj_align(album_container, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(album_container, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(album_container, 0, 0); // Remove border completely
    lv_obj_set_style_radius(album_container, 12, 0);      // More rounded corners for larger size
    lv_obj_set_style_pad_all(album_container, 0, 0);      // Remove padding - let image fill container
    lv_obj_clear_flag(album_container, LV_OBJ_FLAG_SCROLLABLE);

    // Album artwork image (170x170) - fill container completely
    album_img = lv_img_create(album_container);
    lv_obj_set_size(album_img, 170, 170);           // Fill entire container
    lv_obj_align(album_img, LV_ALIGN_CENTER, 0, 0); // Center in container
    lv_obj_clear_flag(album_img, LV_OBJ_FLAG_SCROLLABLE);

    // Show placeholder initially
    lvgl_show_album_placeholder();

    // Track info container (adjusted for 170px album art, centered vertically)
    lv_obj_t *info_container = lv_obj_create(main_screen);
    lv_obj_set_size(info_container, 130, 170);
    lv_obj_align(info_container, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_opa(info_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(info_container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(info_container, LV_OBJ_FLAG_SCROLLABLE);

    // Track name label
    track_label = lv_label_create(info_container);
    lv_label_set_text(track_label, "No track playing");
    lv_obj_set_style_text_color(track_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(track_label, &lv_font_montserrat_14, 0);
    lv_obj_align(track_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_long_mode(track_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(track_label, 120);

    // Artist label
    artist_label = lv_label_create(info_container);
    lv_label_set_text(artist_label, "Unknown Artist");
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0xb3b3b3), 0);
    lv_obj_set_style_text_font(artist_label, &lv_font_montserrat_12, 0);
    lv_obj_align(artist_label, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_label_set_long_mode(artist_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(artist_label, 120);

    // Album label
    album_label = lv_label_create(info_container);
    lv_label_set_text(album_label, "Unknown Album");
    lv_obj_set_style_text_color(album_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(album_label, &lv_font_montserrat_12, 0);
    lv_obj_align(album_label, LV_ALIGN_TOP_LEFT, 0, 45);
    lv_label_set_long_mode(album_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(album_label, 120);

    // Status label (moved to bottom of info container)
    status_label = lv_label_create(info_container);
    lv_label_set_text(status_label, "Paused");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x1db954), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 0, -10);

    Serial0.println("✅ LVGL UI created successfully");
}

// Update WiFi status
void lvgl_update_wifi_status(bool connected)
{
    if (connected)
    {
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x1db954), 0); // Green when connected
    }
    else
    {
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xffa500), 0); // Orange when connecting
    }
}

void lvgl_switch_screen(UIScreen screen)
{
    if (screen == currentScreen)
        return;

    currentScreen = screen;

    switch (screen)
    {
    case SCREEN_MAIN:
        lv_scr_load(main_screen);
        Serial0.println("Switched to main screen");
        break;
    case SCREEN_DEVICES:
        lvgl_show_device_screen();
        Serial0.println("Switched to device screen");
        break;
    }
}