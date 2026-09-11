/* USB serial time sync for the status screen. SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Local wall-clock time as last set over the time port, advanced by uptime.
 * Returns false until a sync has happened since boot. */
bool npv_time_now(uint8_t *hour, uint8_t *minute);
