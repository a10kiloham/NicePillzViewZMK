/*
 * Nice Pillz nice!view status screen - ZMK glue.
 *
 * Portrait layout, header pins at the bottom:
 *
 *      [=====]  87%     battery gauge + percent, or "Charging" when on USB
 *        63             words per minute (refreshed at most every 5 s)
 *      (BT) 1 ok        output: bluetooth logo + profile, or a USB symbol
 *   (1)(2)(3)(4)(5)     active layer
 *     [Caps Lock]       HID lock indicators, stacked
 *
 * Everything is drawn upright into a 68x160 canvas by view_draw.c, then
 * rotated into the panel's native 160x68 buffer. All LVGL work happens on
 * the ZMK display work queue.
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/hid_indicators.h>
#include <zmk/wpm.h>

#include "view_draw.h"

/* HID boot keyboard output report bits */
#define HID_LED_NUM_LOCK BIT(0)
#define HID_LED_CAPS_LOCK BIT(1)
#define HID_LED_SCROLL_LOCK BIT(2)

static lv_color_t upright_buf[NPV_W * NPV_H];
static lv_color_t panel_buf[NPV_PANEL_W * NPV_PANEL_H];
static lv_obj_t *upright_canvas;
static lv_obj_t *panel_canvas;

static struct npv_state state;
static K_MUTEX_DEFINE(state_lock);
static int64_t last_wpm_update;

static void capture_zmk_state(struct npv_state *s) {
    s->battery = zmk_battery_state_of_charge();
    s->charging = zmk_usb_is_powered();

    struct zmk_endpoint_instance ep = zmk_endpoints_selected();
    s->transport = (ep.transport == ZMK_TRANSPORT_USB) ? NPV_TRANSPORT_USB : NPV_TRANSPORT_BLE;
    s->ble_profile = zmk_ble_active_profile_index();
    s->ble_connected = zmk_ble_active_profile_is_connected();
    s->ble_open = zmk_ble_active_profile_is_open();

    s->layer = zmk_keymap_highest_layer_active();

    zmk_hid_indicators_t ind = zmk_hid_indicators_get_current_profile();
    s->num_lock = ind & HID_LED_NUM_LOCK;
    s->caps_lock = ind & HID_LED_CAPS_LOCK;
    s->scroll_lock = ind & HID_LED_SCROLL_LOCK;

    s->inverted = IS_ENABLED(CONFIG_NICEPILLZ_DISPLAY_INVERTED);
}

/* Runs on the display work queue. */
static void redraw_handler(struct k_work *work) {
    struct npv_state snapshot;
    k_mutex_lock(&state_lock, K_FOREVER);
    snapshot = state;
    k_mutex_unlock(&state_lock);

    npv_draw(upright_canvas, &snapshot);
    npv_rotate(upright_buf, panel_buf);
    lv_obj_invalidate(panel_canvas);
}

K_WORK_DEFINE(redraw_work, redraw_handler);

/* Any ZMK state event: re-read everything and redraw. */
static int zmk_event_listener(const zmk_event_t *eh) {
    k_mutex_lock(&state_lock, K_FOREVER);
    capture_zmk_state(&state);
    k_mutex_unlock(&state_lock);
    k_work_submit_to_queue(zmk_display_work_q(), &redraw_work);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(npv_listener, zmk_event_listener);
ZMK_SUBSCRIPTION(npv_listener, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(npv_listener, zmk_usb_conn_state_changed);
ZMK_SUBSCRIPTION(npv_listener, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(npv_listener, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(npv_listener, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(npv_listener, zmk_hid_indicators_changed);

/* Once a second on the display queue: throttled WPM refresh. */
static void tick_handler(struct k_work *work) {
    bool changed = false;
    int64_t now = k_uptime_get();

    k_mutex_lock(&state_lock, K_FOREVER);
    int wpm = zmk_wpm_get_state();
    if (wpm != state.wpm && (now - last_wpm_update) >= CONFIG_NICEPILLZ_WPM_INTERVAL_MS) {
        state.wpm = wpm;
        last_wpm_update = now;
        changed = true;
    }
    k_mutex_unlock(&state_lock);

    if (changed) {
        redraw_handler(NULL);
    }
}

K_WORK_DEFINE(tick_work, tick_handler);

static void tick_timer_cb(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_display_work_q(), &tick_work);
}

K_TIMER_DEFINE(tick_timer, tick_timer_cb, NULL);

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    panel_canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(panel_canvas, panel_buf, NPV_PANEL_W, NPV_PANEL_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(panel_canvas, LV_ALIGN_TOP_LEFT, 0, 0);

    /* scratch canvas for the upright drawing, never shown directly */
    upright_canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(upright_canvas, upright_buf, NPV_W, NPV_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_add_flag(upright_canvas, LV_OBJ_FLAG_HIDDEN);

    k_mutex_lock(&state_lock, K_FOREVER);
    capture_zmk_state(&state);
    state.wpm = zmk_wpm_get_state();
    k_mutex_unlock(&state_lock);
    last_wpm_update = k_uptime_get();

    redraw_handler(NULL);
    k_timer_start(&tick_timer, K_SECONDS(1), K_SECONDS(1));

    return screen;
}
