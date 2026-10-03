/*
 * Nice Pillz nice!view status screen - drawing module.
 * SPDX-License-Identifier: MIT
 */
#include "view_draw.h"

#include <stdio.h>
#include <string.h>

/* ---- vertical layout (upright coordinates, y grows downwards) ---- */
#define BAT_ICON_Y 4
#define BAT_TEXT_Y 24
#define SEP2_Y 41
#define GRAPH_X 4
#define GRAPH_Y 44
#define GRAPH_W 60
#define GRAPH_H 30
#define SEP3_Y 76
#define BT_Y 78
#define DOTS_Y 106
#define LOCKS_Y 123
#define LOCKS_H 12
#define MODS1_Y 137
#define MODS2_Y 148
#define MODS_H 10

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

/* Battery block: gauge (or a charging mark) with the percentage below. (x, y) is the top left
 * of the block and w its width. compact = only the bolt instead of the word "Charging". */
static void draw_battery(lv_obj_t *c, const struct npv_state *st, lv_coord_t x, lv_coord_t y,
                         lv_coord_t w, lv_coord_t icon_x, bool compact) {
    char buf[16];

    if (st->charging) {
        /* lightning bolt replaces the gauge */
        if (compact) {
            text(c, &lv_font_montserrat_22, x, y + BAT_ICON_Y - 2, w, LV_TEXT_ALIGN_CENTER, fg,
                 LV_SYMBOL_CHARGE);
        } else {
            text(c, &lv_font_montserrat_12, x, y + BAT_ICON_Y + 2, w, LV_TEXT_ALIGN_CENTER, fg,
                 LV_SYMBOL_CHARGE " Charging");
        }
    } else {
        /* battery outline, nub and fill */
        rect(c, icon_x, y + BAT_ICON_Y, BAT_ICON_W, BAT_ICON_H, 2, false, 2, fg);
        rect(c, icon_x + BAT_ICON_W, y + BAT_ICON_Y + (BAT_ICON_H - BAT_NUB_H) / 2, BAT_NUB_W,
             BAT_NUB_H, 1, true, 0, fg);
        lv_coord_t inner_w = BAT_ICON_W - 8;
        lv_coord_t fill_w = (inner_w * (st->battery > 100 ? 100 : st->battery)) / 100;
        if (fill_w > 0) {
            rect(c, icon_x + 4, y + BAT_ICON_Y + 4, fill_w, BAT_ICON_H - 8, 0, true, 0, fg);
        }
    }

    snprintf(buf, sizeof(buf), "%u%%", st->battery);
    text(c, &lv_font_montserrat_12, x, y + BAT_TEXT_Y, w, LV_TEXT_ALIGN_CENTER, fg, buf);
}

static void draw_wpm(lv_obj_t *c, const struct npv_state *st, lv_coord_t gx, lv_coord_t gy,
                     lv_coord_t gw, lv_coord_t gh) {
    /* frame */
    rect(c, gx, gy, gw, gh, 2, false, 1, fg);

    /* history as a polyline, auto-scaled, newest sample on the right */
    const lv_coord_t x0 = gx + 2, x1 = gx + gw - 3;
    const lv_coord_t y_top = gy + 2, y_bot = gy + gh - 3;
    uint16_t vmax = 30;
    for (int i = 0; i < NPV_WPM_POINTS; i++) {
        if (st->wpm_hist[i] > vmax) {
            vmax = st->wpm_hist[i];
        }
    }
    lv_point_t pts[NPV_WPM_POINTS];
    for (int i = 0; i < NPV_WPM_POINTS; i++) {
        pts[i].x = x0 + (i * (x1 - x0)) / (NPV_WPM_POINTS - 1);
        pts[i].y = y_bot - ((y_bot - y_top) * st->wpm_hist[i]) / vmax;
    }
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = fg;
    d.width = 1;
    lv_canvas_draw_line(c, pts, NPV_WPM_POINTS, &d);

    /* current value in the top-left corner of the frame, on a clear patch */
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", st->wpm);
    lv_point_t sz;
    lv_txt_get_size(&sz, buf, &lv_font_montserrat_10, 0, 0, LV_COORD_MAX, 0);
    rect(c, gx + 2, gy + 2, sz.x + 3, 10, 0, true, 0, bg);
    text(c, &lv_font_montserrat_10, gx + 3, gy + 1, 24, LV_TEXT_ALIGN_LEFT, fg, buf);
}

/* Output block, 22 + 20 + 16 px wide from x + pad; a USB symbol is centred in (x, w). */
static void draw_output(lv_obj_t *c, const struct npv_state *st, lv_coord_t x, lv_coord_t y,
                        lv_coord_t w, lv_coord_t pad) {
    if (st->transport == NPV_TRANSPORT_USB) {
        text(c, &lv_font_montserrat_22, x, y, w, LV_TEXT_ALIGN_CENTER, fg, LV_SYMBOL_USB);
        return;
    }

    /* bluetooth logo shifted left, the active profile as a numeral of the same size next to
     * it, and a small connection mark in the corner: tick = connected, cross = not connected,
     * nothing = profile is free and advertising */
    lv_coord_t lx = x + pad;
    text(c, &lv_font_montserrat_22, lx, y, 22, LV_TEXT_ALIGN_CENTER, fg, LV_SYMBOL_BLUETOOTH);

    char num[4];
    snprintf(num, sizeof(num), "%u", st->ble_profile + 1);
    text(c, &lv_font_montserrat_22, lx + 22, y, 20, LV_TEXT_ALIGN_CENTER, fg, num);

    const char *status = st->ble_connected ? LV_SYMBOL_OK : (st->ble_open ? "" : LV_SYMBOL_CLOSE);
    text(c, &lv_font_montserrat_10, lx + 42, y + 8, 16, LV_TEXT_ALIGN_CENTER, fg, status);
}

static void draw_layer_dots(lv_obj_t *c, const struct npv_state *st, lv_coord_t x0,
                            lv_coord_t y) {
    for (int i = 0; i < 5; i++) {
        lv_coord_t cx = x0 + i * DOT_PITCH;
        lv_coord_t cy = y + DOT_R;
        bool active = (st->layer == i);
        char num[2] = {'1' + i, 0};

        rect(c, cx - DOT_R, cy - DOT_R, 2 * DOT_R + 1, 2 * DOT_R + 1, LV_RADIUS_CIRCLE, active, 1,
             fg);
        text(c, &lv_font_montserrat_10, cx - DOT_R, cy - 6, 2 * DOT_R + 1, LV_TEXT_ALIGN_CENTER,
             active ? bg : fg, num);
    }
}

/* Lock indicators as one row of filled boxes in (x, w), only the active locks are shown. A
 * single lock gets the full width and its full name, several share the row with short names,
 * or just initials when even those do not fit. */
static void draw_locks(lv_obj_t *c, const struct npv_state *st, lv_coord_t x, lv_coord_t y,
                       lv_coord_t w, lv_coord_t h) {
    static const char *const full[3] = {"Caps Lock", "Num Lock", "Scrl Lock"};
    static const char *const brief[3] = {"Caps", "Num", "Scrl"};
    static const char *const letter[3] = {"C", "N", "S"};
    const bool on[3] = {st->caps_lock, st->num_lock, st->scroll_lock};
    int n = 0;
    for (int i = 0; i < 3; i++) {
        n += on[i];
    }
    if (n == 0) {
        return;
    }

    const lv_coord_t gap = 2;
    const lv_coord_t box_w = (w - (n - 1) * gap) / n;
    const lv_font_t *font = (n == 1) ? &lv_font_montserrat_10 : &lv_font_montserrat_8;
    lv_coord_t text_dy = (n == 1) ? (h - 12) / 2 : (h - 10) / 2 - 1;

    /* portrait with three locks: 20 px boxes, so initials for all of them */
    bool initials = false;
    for (int i = 0; i < 3; i++) {
        const char *label = (n == 1) ? full[i] : brief[i];
        if (on[i] && lv_txt_get_width(label, strlen(label), font, 0, 0) > box_w - 2) {
            initials = true;
        }
    }

    lv_coord_t bx = x;
    for (int i = 0; i < 3; i++) {
        if (!on[i]) {
            continue;
        }
        const char *label = initials ? letter[i] : (n == 1) ? full[i] : brief[i];
        rect(c, bx, y, box_w, h, 2, true, 0, fg);
        text(c, font, bx, y + text_dy, box_w, LV_TEXT_ALIGN_CENTER, bg, label);
        bx += box_w + gap;
    }
}

/* Held modifiers and the Program key as a text row spread over (x, w); the active ones are
 * drawn as filled boxes. first/count select which of Ctrl, Alt, Shift, Win, Prgm to draw, so
 * the portrait layout can split them over two rows. */
static void draw_mods(lv_obj_t *c, const struct npv_state *st, lv_coord_t x, lv_coord_t y,
                      lv_coord_t w, lv_coord_t h, const lv_font_t *font, int first, int count) {
    static const struct {
        const char *label;
        uint8_t bit;
    } m[5] = {{"Ctrl", NPV_MOD_CTRL},
              {"Alt", NPV_MOD_ALT},
              {"Shift", NPV_MOD_SHIFT},
              {"Win", NPV_MOD_GUI},
              {"Prgm", NPV_MOD_PRGM}};
    const lv_coord_t pad = 2; /* box padding either side of the text */

    lv_coord_t tw[5];
    lv_coord_t total = 0;
    for (int i = first; i < first + count; i++) {
        tw[i] = lv_txt_get_width(m[i].label, strlen(m[i].label), font, 0, 0) + 2 * pad;
        total += tw[i];
    }
    lv_coord_t gap = count > 1 ? (w - total) / (count - 1) : 0;
    if (gap < 1) {
        gap = 1;
    }
    lv_coord_t text_dy = (h - lv_font_get_line_height(font)) / 2;

    lv_coord_t bx = x + (w - total - gap * (count - 1)) / 2;
    for (int i = first; i < first + count; i++) {
        bool on = st->mods & m[i].bit;
        if (on) {
            rect(c, bx, y, tw[i], h, 2, true, 0, fg);
        }
        text(c, font, bx, y + text_dy, tw[i], LV_TEXT_ALIGN_CENTER, on ? bg : fg, m[i].label);
        bx += tw[i] + gap;
    }
}

/* Portrait, 68 x 160, header pins at the bottom. */
void npv_draw(lv_obj_t *c, const struct npv_state *st) {
    fg = st->inverted ? lv_color_white() : lv_color_black();
    bg = st->inverted ? lv_color_black() : lv_color_white();

    lv_canvas_fill_bg(c, bg, LV_OPA_COVER);

    draw_battery(c, st, 0, 0, NPV_W, BAT_ICON_X, false);
    hline(c, SEP2_Y, 6, NPV_W - 7);
    draw_wpm(c, st, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H);
    hline(c, SEP3_Y, 6, NPV_W - 7);
    draw_output(c, st, 0, BT_Y, NPV_W, 6);
    draw_layer_dots(c, st, DOT_X0, DOTS_Y);
    draw_locks(c, st, LOCK_BOX_X, LOCKS_Y, LOCK_BOX_W, LOCKS_H);
    draw_mods(c, st, 1, MODS1_Y, NPV_W - 2, MODS_H, &lv_font_montserrat_8, 0, 3);
    draw_mods(c, st, 1, MODS2_Y, NPV_W - 2, MODS_H, &lv_font_montserrat_8, 3, 2);
}

/*
 * Landscape, 160 x 68, drawn in the panel's native orientation (header pins on the left). The
 * display is mounted with the pins on the right, so the caller turns the result with npv_flip():
 *
 *   battery | WPM graph | output          top band
 *   --------------------------------
 *   layer dots      | lock boxes        bottom band, first row
 *   Ctrl Alt Shift Win Prgm            bottom band, second row
 */
#define L_SEP_Y 38
#define L_BAT_W 50
#define L_GRAPH_X 52
#define L_GRAPH_W 46
#define L_GRAPH_H 31
#define L_OUT_X 101
#define L_DOTS_Y 40
#define L_LOCK_X 72
#define L_LOCK_Y 41
#define L_LOCK_H 12
#define L_LOCK_W (NPV_PANEL_W - L_LOCK_X - 2)
#define L_MODS_Y 54
#define L_MODS_H 14

void npv_draw_landscape(lv_obj_t *c, const struct npv_state *st) {
    fg = st->inverted ? lv_color_white() : lv_color_black();
    bg = st->inverted ? lv_color_black() : lv_color_white();

    lv_canvas_fill_bg(c, bg, LV_OPA_COVER);

    draw_battery(c, st, 0, -2, L_BAT_W, 2, true);
    draw_wpm(c, st, L_GRAPH_X, 3, L_GRAPH_W, L_GRAPH_H);
    draw_output(c, st, L_OUT_X, 6, NPV_PANEL_W - L_OUT_X, 0);
    hline(c, L_SEP_Y, 4, NPV_PANEL_W - 5);
    draw_layer_dots(c, st, DOT_X0, L_DOTS_Y);
    draw_locks(c, st, L_LOCK_X, L_LOCK_Y, L_LOCK_W, L_LOCK_H);
    draw_mods(c, st, 2, L_MODS_Y, NPV_PANEL_W - 4, L_MODS_H, &lv_font_montserrat_12, 0, 5);
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

void npv_flip(lv_color_t *panel) {
    for (int i = 0, j = NPV_PANEL_W * NPV_PANEL_H - 1; i < j; i++, j--) {
        lv_color_t tmp = panel[i];
        panel[i] = panel[j];
        panel[j] = tmp;
    }
}
