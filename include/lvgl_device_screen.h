#ifndef LVGL_DEVICE_SCREEN_H
#define LVGL_DEVICE_SCREEN_H

#include <lvgl.h>
#include "spotify_manager.h"

// Device screen management
extern lv_obj_t *device_screen;

void lvgl_create_device_screen();
void lvgl_update_device_list(SpotifyDevice devices[], int deviceCount);
void lvgl_navigate_devices(int direction); // -1 for up, 1 for down
void lvgl_select_current_device();
void lvgl_show_device_screen();
void lvgl_hide_device_screen();
void lvgl_show_main_screen();

#endif // LVGL_DEVICE_SCREEN_H