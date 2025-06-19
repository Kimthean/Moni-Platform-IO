#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include <lvgl.h>
#include "ui_setup.h"

// Display initialization
void lvgl_init_display();

// Display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

#endif // LVGL_DISPLAY_H 