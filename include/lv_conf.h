#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   LVGL 8.x CONFIGURATION
 *====================*/

/* Color depth for ESP32 TFT */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1

/* Memory settings */
#define LV_MEM_SIZE (48U * 1024U)

/* HAL settings for Arduino */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/* Enable built-in fonts */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1

#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Enable widgets */
#define LV_USE_LABEL 1

/* Essential LVGL 8.x settings */
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#endif /*LV_CONF_H*/