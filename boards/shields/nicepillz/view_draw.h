/*
 * Nice Pillz nice!view status screen - drawing module.
 *
 * Pure LVGL, no Zephyr/ZMK dependencies, so the exact same code can be
 * compiled on a host to render previews (see tools/preview/).
 *
 * The screen is designed upright (portrait, 68 wide x 160 tall, pins at the
 * bottom) and drawn into a canvas of that size. npv_rotate() then turns it
 * into the panel's native 160x68 landscape buffer.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

#define NPV_W 68        /* upright width  */
#define NPV_H 160       /* upright height */
#define NPV_PANEL_W 160 /* panel native width  */
#define NPV_PANEL_H 68  /* panel native height */

enum npv_transport { NPV_TRANSPORT_USB, NPV_TRANSPORT_BLE };

struct npv_state {
    uint8_t battery; /* percent */
    bool charging;   /* USB power present */

    uint16_t wpm;

    enum npv_transport transport;
    uint8_t ble_profile; /* 0-based */
    bool ble_connected;
    bool ble_open; /* profile has no bond yet (advertising) */

    uint8_t layer; /* highest active layer, 0-based */

    bool caps_lock;
    bool num_lock;
    bool scroll_lock;

    bool inverted; /* white on black */
};

/* canvas must be NPV_W x NPV_H, LV_IMG_CF_TRUE_COLOR */
void npv_draw(lv_obj_t *canvas, const struct npv_state *st);

/* upright: NPV_W*NPV_H pixels, panel: NPV_PANEL_W*NPV_PANEL_H pixels */
void npv_rotate(const lv_color_t *upright, lv_color_t *panel);
