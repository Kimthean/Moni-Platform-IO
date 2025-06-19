#ifndef LVGL_ALBUM_ART_H
#define LVGL_ALBUM_ART_H

#include <Arduino.h>
#include <lvgl.h>

// Album artwork functions
void lvgl_download_and_set_album_art(const String& imageUrl);
void lvgl_set_album_art(const uint8_t* img_data, size_t img_size);
void lvgl_show_album_placeholder();
void lvgl_show_loading_indicator();
void lvgl_cleanup_album_art();

// Asynchronous loading
void lvgl_lazy_load_album_art(const String& imageUrl);
void lvgl_process_pending_images();

// JPEG decoding callback
bool tjpg_output_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

#endif // LVGL_ALBUM_ART_H 