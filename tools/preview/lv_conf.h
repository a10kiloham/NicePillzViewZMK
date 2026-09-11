/* LVGL configuration for the host-side preview build (matches the firmware:
 * 1-bit colour, canvas drawing, Montserrat 8/10/12/22). */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 1
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128U * 1024U)
#define LV_USE_LOG 0
#define LV_TICK_CUSTOM 0
#define LV_DPI_DEF 161
#define LV_DRAW_COMPLEX 1
#define LV_USE_IMG 1
#define LV_USE_CANVAS 1
#define LV_USE_LABEL 1
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_BASIC 0
#define LV_USE_THEME_MONO 1
#define LV_FONT_MONTSERRAT_8 1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_DEFAULT &lv_font_montserrat_12

#endif
