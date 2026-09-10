/*
 * Horizontal (landscape) status screen for the nice!view on the Nice Pillz.
 *
 * Built from ZMK's stock display widgets, laid out for the 160x68 display
 * in its native landscape orientation:
 *
 *   +----------------------------------------+
 *   | [output: USB / BT profile]  [charge %] |
 *   |                                        |
 *   | [layer]                       WPM  nnn |
 *   +----------------------------------------+
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>

#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/wpm_status.h>

#define WPM_GROUP_WIDTH 72
#define WPM_GROUP_HEIGHT 22

static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_wpm_status wpm_status_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Top row: connection/output status on the left, battery on the right. */
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    /* Bottom row: active layer on the left, words per minute on the right. */
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /*
     * The stock WPM widget is a bare number that pins itself to the bottom
     * right of its parent, so give it a small container with a "WPM" caption.
     */
    lv_obj_t *wpm_group = lv_obj_create(screen);
    lv_obj_remove_style_all(wpm_group);
    lv_obj_set_size(wpm_group, WPM_GROUP_WIDTH, WPM_GROUP_HEIGHT);
    lv_obj_align(wpm_group, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lv_obj_t *wpm_caption = lv_label_create(wpm_group);
    lv_label_set_text_static(wpm_caption, "WPM");
    lv_obj_set_style_text_font(wpm_caption, lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(wpm_caption, LV_ALIGN_BOTTOM_LEFT, 0, -1);

    zmk_widget_wpm_status_init(&wpm_status_widget, wpm_group);
    lv_obj_align(zmk_widget_wpm_status_obj(&wpm_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    return screen;
}
