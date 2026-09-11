/*
 * USB serial time sync.
 *
 * The board has no clock, so the host sets it: it writes "T<local epoch
 * seconds>\n" to the CDC ACM port declared as cdc_acm_uart_time in the
 * overlay, and the board answers "OK HH:MM\n". The time then runs from the
 * kernel uptime until the next sync. Deep sleep resets the board and clears
 * it. See scripts/nicepillz-sync-time.sh.
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h>

#include "time_sync.h"

LOG_MODULE_REGISTER(npv_time, CONFIG_ZMK_LOG_LEVEL);

#define TIME_UART_NODE DT_NODELABEL(cdc_acm_uart_time)

static const struct device *const time_uart = DEVICE_DT_GET(TIME_UART_NODE);

static int64_t epoch_at_sync;  /* local seconds since 1970 */
static int64_t uptime_at_sync; /* k_uptime_get() at that moment */
static bool synced;

static char line[32];
static size_t line_len;

bool npv_time_now(uint8_t *hour, uint8_t *minute) {
    unsigned int key = irq_lock();
    bool valid = synced;
    int64_t epoch = epoch_at_sync;
    int64_t uptime = uptime_at_sync;
    irq_unlock(key);

    if (!valid) {
        return false;
    }
    int64_t now = epoch + (k_uptime_get() - uptime) / 1000;
    int64_t secs = now % 86400;
    if (secs < 0) {
        secs += 86400;
    }
    *hour = secs / 3600;
    *minute = (secs % 3600) / 60;
    return true;
}

static void reply_handler(struct k_work *work) {
    char buf[16];
    uint8_t h, m;
    int n;
    if (npv_time_now(&h, &m)) {
        n = snprintf(buf, sizeof(buf), "OK %02u:%02u\n", h, m);
    } else {
        n = snprintf(buf, sizeof(buf), "ERR\n");
    }
    for (int i = 0; i < n; i++) {
        uart_poll_out(time_uart, buf[i]);
    }
}

K_WORK_DEFINE(reply_work, reply_handler);

static void handle_line(void) {
    line[line_len] = '\0';
    if (line[0] == 'T') {
        char *end;
        long long v = strtoll(line + 1, &end, 10);
        if (end != line + 1 && v > 0) {
            epoch_at_sync = v;
            uptime_at_sync = k_uptime_get();
            synced = true;
            LOG_INF("time set: %lld", v);
        }
    }
    k_work_submit(&reply_work);
}

static void uart_cb(const struct device *dev, void *user_data) {
    uint8_t c;
    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        while (uart_fifo_read(dev, &c, 1) == 1) {
            if (c == '\n' || c == '\r') {
                if (line_len > 0) {
                    handle_line();
                }
                line_len = 0;
            } else if (line_len < sizeof(line) - 1) {
                line[line_len++] = c;
            } else {
                line_len = 0; /* garbage: start over */
            }
        }
    }
}

static int npv_time_init(void) {
    if (!device_is_ready(time_uart)) {
        LOG_WRN("time port not ready");
        return 0;
    }
    uart_irq_callback_user_data_set(time_uart, uart_cb, NULL);
    uart_irq_rx_enable(time_uart);
    return 0;
}

SYS_INIT(npv_time_init, APPLICATION, 99);
