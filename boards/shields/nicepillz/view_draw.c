/*
 * Nice Pillz nice!view status screen - drawing module.
 * SPDX-License-Identifier: MIT
 */
#include "view_draw.h"

#include <stdio.h>
#include <string.h>

/* ---- vertical layout (upright coordinates, y grows downwards) ---- */
#define BAT_ICON_Y 14
#define BAT_TEXT_Y 36
#define SEP2_Y 58
#define WPM_Y 62
#define SEP3_Y 90
#define BT_Y 93
#define DOTS_Y 117
#define LOCKS_BOTTOM NPV_H

#define BAT_ICON_X 10
#define BAT_ICON_W 42
#define BAT_ICON_H 18
#define BAT_NUB_W 4
#define BAT_NUB_H 8

#define DOT_R 6
#define DOT_PITCH 13
#define DOT_X0 8

#define LOCK_BOX_X 4
#define LOCK_BOX_W (NPV_W - 2 * LOCK_BOX_X)

static lv_color_t fg;
static lv_color_t bg;

static void text(lv_obj_t *c, const lv_font_t *font, lv_coord_t x, lv_coord_t y, lv_coord_t w,
                 lv_text_align_t align, lv_color_t color, const char *s) {
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.font = font;
    d.color = color;
    d.align = align;
    lv_canvas_draw_text(c, x, y, w, &d, s);
}

static void rect(lv_obj_t *c, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                 lv_coord_t radius, bool filled, lv_coord_t border, lv_color_t color) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = radius;
    d.bg_color = color;
    d.bg_opa = filled ? LV_OPA_COVER : LV_OPA_TRANSP;
    d.border_color = color;
    d.border_width = border;
    d.border_opa = border > 0 ? LV_OPA_COVER : LV_OPA_TRANSP;
    lv_canvas_draw_rect(c, x, y, w, h, &d);
}

static void hline(lv_obj_t *c, lv_coord_t y, lv_coord_t x1, lv_coord_t x2) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = fg;
    d.width = 1;
    lv_point_t p[2] = {{x1, y}, {x2, y}};
    lv_canvas_draw_line(c, p, 2, &d);
}

static void draw_battery(lv_obj_t *c, const struct npv_state *st) {
    char buf[16];

    if (st->charging) {
        /* lightning bolt replaces the gauge */
        text(c, &lv_font_montserrat_12, 0, BAT_ICON_Y + 2, NPV_W, LV_TEXT_ALIGN_CENTER, fg,
             LV_SYMBOL_CHARGE " Charging");
    } else {
        /* battery outline, nub and fill */
        rect(c, BAT_ICON_X, BAT_ICON_Y, BAT_ICON_W, BAT_ICON_H, 2, false, 2, fg);
        rect(c, BAT_ICON_X + BAT_ICON_W, BAT_ICON_Y + (BAT_ICON_H - BAT_NUB_H) / 2, BAT_NUB_W,
             BAT_NUB_H, 1, true, 0, fg);
        lv_coord_t inner_w = BAT_ICON_W - 8;
        lv_coord_t fill_w = (inner_w * (st->battery > 100 ? 100 : st->battery)) / 100;
        if (fill_w > 0) {
            rect(c, BAT_ICON_X + 4, BAT_ICON_Y + 4, fill_w, BAT_ICON_H - 8, 0, true, 0, fg);
        }
    }

    snprintf(buf, sizeof(buf), "%u%%", st->battery);
    text(c, &lv_font_montserrat_12, 0, BAT_TEXT_Y, NPV_W, LV_TEXT_ALIGN_CENTER, fg, buf);
}

static void draw_wpm(lv_obj_t *c, const struct npv_state *st) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", st->wpm);
    text(c, &lv_font_montserrat_22, 0, WPM_Y, NPV_W, LV_TEXT_ALIGN_CENTER, fg, buf);
}

static void draw_output(lv_obj_t *c, const struct npv_state *st) {
    if (st->transport == NPV_TRANSPORT_USB) {
        text(c, &lv_font_montserrat_22, 0, BT_Y, NPV_W, LV_TEXT_ALIGN_CENTER, fg, LV_SYMBOL_USB);
        return;
    }

    /* medium bluetooth logo on the left, profile number + state on the right */
    text(c, &lv_font_montserrat_22, 10, BT_Y, 26, LV_TEXT_ALIGN_CENTER, fg, LV_SYMBOL_BLUETOOTH);

    char num[4];
    snprintf(num, sizeof(num), "%u", st->ble_profile + 1);
    text(c, &lv_font_montserrat_12, 38, BT_Y, 22, LV_TEXT_ALIGN_CENTER, fg, num);
    const char *status = st->ble_connected ? LV_SYMBOL_OK : (st->ble_open ? "" : LV_SYMBOL_CLOSE);
    text(c, &lv_font_montserrat_10, 38, BT_Y + 14, 22, LV_TEXT_ALIGN_CENTER, fg, status);
}

static void draw_layer_dots(lv_obj_t *c, const struct npv_state *st) {
    for (int i = 0; i < 5; i++) {
        lv_coord_t cx = DOT_X0 + i * DOT_PITCH;
        lv_coord_t cy = DOTS_Y + DOT_R;
        bool active = (st->layer == i);
        char num[2] = {'1' + i, 0};

        rect(c, cx - DOT_R, cy - DOT_R, 2 * DOT_R + 1, 2 * DOT_R + 1, LV_RADIUS_CIRCLE, active, 1,
             fg);
        text(c, &lv_font_montserrat_10, cx - DOT_R, cy - 6, 2 * DOT_R + 1, LV_TEXT_ALIGN_CENTER,
             active ? bg : fg, num);
    }
}

static void draw_locks(lv_obj_t *c, const struct npv_state *st) {
    const char *labels[3];
    int n = 0;
    if (st->caps_lock) {
        labels[n++] = "Caps Lock";
    }
    if (st->num_lock) {
        labels[n++] = "Num Lock";
    }
    if (st->scroll_lock) {
        labels[n++] = "Scrl Lock";
    }
    if (n == 0) {
        return;
    }

    /* stack from the bottom; shrink the rows when all three are on */
    lv_coord_t row_h = (n >= 3) ? 9 : 12;
    const lv_font_t *font = (n >= 3) ? &lv_font_montserrat_8 : &lv_font_montserrat_10;
    lv_coord_t text_dy = (n >= 3) ? -1 : 0;

    for (int i = 0; i < n; i++) {
        lv_coord_t y = LOCKS_BOTTOM - (n - i) * row_h;
        rect(c, LOCK_BOX_X, y, LOCK_BOX_W, row_h - 1, 2, true, 0, fg);
        text(c, font, LOCK_BOX_X, y + text_dy, LOCK_BOX_W, LV_TEXT_ALIGN_CENTER, bg, labels[i]);
    }
}

void npv_draw(lv_obj_t *c, const struct npv_state *st) {
    fg = st->inverted ? lv_color_white() : lv_color_black();
    bg = st->inverted ? lv_color_black() : lv_color_white();

    lv_canvas_fill_bg(c, bg, LV_OPA_COVER);

    draw_battery(c, st);
    hline(c, SEP2_Y, 6, NPV_W - 7);
    draw_wpm(c, st);
    hline(c, SEP3_Y, 6, NPV_W - 7);
    draw_output(c, st);
    draw_layer_dots(c, st);
    draw_locks(c, st);
}

/*
 * Upright (u, v) -> panel (x, y): rotate 90 degrees clockwise, which is what
 * ZMK's own nice!view screen does. The panel's x = 159 end is the end away
 * from the header pins, i.e. the top of the upright picture.
 */
void npv_rotate(const lv_color_t *upright, lv_color_t *panel) {
    for (int y = 0; y < NPV_PANEL_H; y++) {
        for (int x = 0; x < NPV_PANEL_W; x++) {
            panel[y * NPV_PANEL_W + x] = upright[(NPV_PANEL_W - 1 - x) * NPV_W + y];
        }
    }
}
