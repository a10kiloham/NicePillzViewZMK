/* Renders the Nice Pillz status screen with the real drawing code on the host.
 * Writes one PBM per scene (upright, as seen on the keyboard) plus one PBM of
 * the raw panel buffer to prove the rotation. */
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "view_draw.h"

static lv_color_t upright_buf[NPV_W * NPV_H];
static lv_color_t panel_buf[NPV_PANEL_W * NPV_PANEL_H];

static void flush_cb(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *p) { lv_disp_flush_ready(d); }

static void save_pbm(const char *path, const lv_color_t *buf, int w, int h) {
    FILE *f = fopen(path, "wb");
    fprintf(f, "P4\n%d %d\n", w, h);
    for (int y = 0; y < h; y++) {
        for (int xb = 0; xb < (w + 7) / 8; xb++) {
            unsigned char byte = 0;
            for (int b = 0; b < 8; b++) {
                int x = xb * 8 + b;
                if (x < w && buf[y * w + x].full == 0) byte |= 0x80 >> b; /* 1 = black */
            }
            fputc(byte, f);
        }
    }
    fclose(f);
}

struct scene { const char *name; struct npv_state st; };

static void fill_hist(struct npv_state *st, const uint8_t *src, int n) {
    for (int i = 0; i < NPV_WPM_POINTS; i++) st->wpm_hist[i] = src[i % n];
}

int main(void) {
    lv_init();
    static lv_disp_draw_buf_t db; static lv_color_t dbuf[NPV_PANEL_W * NPV_PANEL_H];
    lv_disp_draw_buf_init(&db, dbuf, NULL, NPV_PANEL_W * NPV_PANEL_H);
    static lv_disp_drv_t drv; lv_disp_drv_init(&drv);
    drv.hor_res = NPV_PANEL_W; drv.ver_res = NPV_PANEL_H; drv.flush_cb = flush_cb; drv.draw_buf = &db;
    lv_disp_drv_register(&drv);

    lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, upright_buf, NPV_W, NPV_H, LV_IMG_CF_TRUE_COLOR);

    struct scene scenes[] = {
        {"typical",  {.battery = 82, .wpm = 63,
                      .transport = NPV_TRANSPORT_BLE, .ble_profile = 0, .ble_connected = true, .layer = 0}},
        {"charging", {.battery = 57, .charging = true, .wpm = 0,
                      .transport = NPV_TRANSPORT_USB, .layer = 1, .caps_lock = true}},
        {"locks",    {.battery = 15, .wpm = 120,
                      .transport = NPV_TRANSPORT_BLE, .ble_profile = 2, .ble_connected = false, .layer = 4,
                      .caps_lock = true, .num_lock = true, .scroll_lock = true}},
        {"unsynced", {.battery = 100, .wpm = 41,
                      .transport = NPV_TRANSPORT_BLE, .ble_profile = 1, .ble_open = true, .layer = 2,
                      .num_lock = true, .scroll_lock = true}},
        {"inverted", {.battery = 64, .wpm = 88,
                      .transport = NPV_TRANSPORT_BLE, .ble_profile = 0, .ble_connected = true, .layer = 3,
                      .inverted = true, .caps_lock = true}},
    };
    static const uint8_t h1[] = {0, 0, 12, 35, 52, 61, 58, 66, 70, 63, 59, 64, 71, 68, 55, 40, 30, 44, 58, 62, 65, 60, 63, 63};
    static const uint8_t h2[] = {0};
    static const uint8_t h3[] = {20, 40, 80, 110, 120, 118, 95, 60, 30, 10, 0, 0, 25, 70, 100, 120, 115, 90, 70, 60, 80, 110, 120, 120};
    static const uint8_t h4[] = {41, 38, 44, 40, 42, 39, 45, 41};
    fill_hist(&scenes[0].st, h1, sizeof h1); fill_hist(&scenes[1].st, h2, 1);
    fill_hist(&scenes[2].st, h3, sizeof h3); fill_hist(&scenes[3].st, h4, sizeof h4); fill_hist(&scenes[4].st, h1, sizeof h1);
    scenes[0].st.mods = NPV_MOD_CTRL; scenes[2].st.mods = NPV_MOD_CTRL | NPV_MOD_SHIFT; scenes[4].st.mods = NPV_MOD_ALT | NPV_MOD_GUI;
    char path[64];
    for (size_t i = 0; i < sizeof(scenes) / sizeof(scenes[0]); i++) {
        npv_draw(canvas, &scenes[i].st);
        snprintf(path, sizeof(path), "build/%s.pbm", scenes[i].name);
        save_pbm(path, upright_buf, NPV_W, NPV_H);
        if (i == 0) {
            npv_rotate(upright_buf, panel_buf);
            save_pbm("build/panel_raw.pbm", panel_buf, NPV_PANEL_W, NPV_PANEL_H);
        }
    }
    return 0;
}
