#ifndef LVGL_UI_COMPONENTS_H
#define LVGL_UI_COMPONENTS_H

#include <lvgl.h>

// Screen management
enum UIScreen
{
    SCREEN_MAIN = 0,
    SCREEN_DEVICES = 1
};

extern UIScreen currentScreen;

// LVGL UI objects (global references)
extern lv_obj_t *main_screen;
extern lv_obj_t *album_img;
extern lv_obj_t *track_label;
extern lv_obj_t *artist_label;
extern lv_obj_t *album_label;
extern lv_obj_t *status_label;
extern lv_obj_t *wifi_status_label;

// UI creation and management
void lvgl_create_ui();
void lvgl_update_wifi_status(bool connected);
void lvgl_switch_screen(UIScreen screen);

#endif // LVGL_UI_COMPONENTS_H