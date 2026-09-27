/*
 * Makes the compiled keymap the default after flashing.
 *
 * ZMK Studio stores its edits in the settings partition, which survives a firmware flash, so an
 * old Studio keymap would keep overriding a newly flashed one. The build stamps the firmware
 * with a hash of the keymap file (NICEPILLZ_KEYMAP_REV). On boot, if the hash stored in settings
 * is different, the stored Studio keymap is discarded and the new hash is saved. Studio edits
 * made afterwards are kept until a firmware with a different keymap is flashed.
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define REV_SETTING "nicepillz/keymap_rev"

static uint32_t stored_rev;
static bool checked;

static void keymap_rev_work_handler(struct k_work *work) {
    const uint32_t rev = NICEPILLZ_KEYMAP_REV;

    LOG_INF("Keymap changed (%08x -> %08x), discarding the stored Studio keymap", stored_rev, rev);

    int ret = zmk_keymap_reset_settings();
    if (ret < 0) {
        LOG_ERR("Failed to reset the keymap settings (%d)", ret);
        return;
    }

    ret = settings_save_one(REV_SETTING, &rev, sizeof(rev));
    if (ret < 0) {
        LOG_ERR("Failed to store the keymap revision (%d)", ret);
        return;
    }
    stored_rev = rev;
}

static K_WORK_DEFINE(keymap_rev_work, keymap_rev_work_handler);

static int keymap_rev_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (settings_name_steq(name, "keymap_rev", NULL)) {
        if (len != sizeof(stored_rev)) {
            return -EINVAL;
        }
        ssize_t ret = read_cb(cb_arg, &stored_rev, sizeof(stored_rev));
        return ret < 0 ? ret : 0;
    }
    return -ENOENT;
}

/* Called once every stored setting has been loaded. */
static int keymap_rev_commit(void) {
    if (checked) {
        return 0;
    }
    checked = true;

    if (stored_rev != NICEPILLZ_KEYMAP_REV) {
        k_work_submit(&keymap_rev_work);
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(nicepillz, "nicepillz", NULL, keymap_rev_set, keymap_rev_commit,
                               NULL);
