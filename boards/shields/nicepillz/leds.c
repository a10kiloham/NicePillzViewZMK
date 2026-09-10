/*
 * Status LEDs for the Nice Pillz.
 *
 *   led_0 / led-caps : power LED, driven by the 74HC595 output QA
 *   led_1 / led-num  : BLE LED, driven directly by P1.02
 *
 * The 74HC595 is reached over SPI, and its GPIO driver refuses to run from
 * interrupt context (it returns -EWOULDBLOCK). All LED writes are therefore
 * funnelled through a single work item on the system work queue; the idle
 * blink timer only flips a phase flag and queues that work.
 *
 * Behavior:
 *   active : power LED on, BLE LED mirrors the active profile connection
 *   idle   : power LED slow blink (1s on / 2s off), BLE LED off
 *   sleep  : both off
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/activity_state_changed.h>

LOG_MODULE_REGISTER(nicepillz_leds, CONFIG_ZMK_LOG_LEVEL);

#define LED_GPIO_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(gpio_leds)

#define LED_PWR DT_NODE_CHILD_IDX(DT_ALIAS(led_caps))
#define LED_BLE DT_NODE_CHILD_IDX(DT_ALIAS(led_num))

/* Idle blink timing */
#define IDLE_BLINK_ON_MS 1000
#define IDLE_BLINK_OFF_MS 2000

static const struct device *led_dev = DEVICE_DT_GET(LED_GPIO_NODE_ID);

static enum zmk_activity_state activity = ZMK_ACTIVITY_ACTIVE;
static bool blink_phase_on;

static void set_led(uint32_t led, bool on) {
    int ret = on ? led_on(led_dev, led) : led_off(led_dev, led);
    if (ret < 0) {
        LOG_WRN("Failed to set LED %u: %d", led, ret);
    }
}

/* --- Apply the LED state for the current activity/BLE state --- */

static void led_update_handler(struct k_work *work) {
    switch (activity) {
    case ZMK_ACTIVITY_ACTIVE:
        set_led(LED_PWR, true);
        set_led(LED_BLE, zmk_ble_active_profile_is_connected());
        break;
    case ZMK_ACTIVITY_IDLE:
        set_led(LED_PWR, blink_phase_on);
        set_led(LED_BLE, false);
        break;
    case ZMK_ACTIVITY_SLEEP:
        set_led(LED_PWR, false);
        set_led(LED_BLE, false);
        break;
    }
}

K_WORK_DEFINE(led_update_work, led_update_handler);

/*
 * Apply immediately when called from a thread (ZMK raises its events from
 * thread context, and on the way to deep sleep there may be no chance for
 * deferred work to run), otherwise defer to the system work queue.
 */
static void led_request_update(void) {
    if (k_is_in_isr()) {
        k_work_submit(&led_update_work);
    } else {
        led_update_handler(NULL);
    }
}

/* --- Idle slow blink timer --- */

static void idle_blink_handler(struct k_timer *timer);
K_TIMER_DEFINE(idle_blink_timer, idle_blink_handler, NULL);

static void idle_blink_handler(struct k_timer *timer) {
    blink_phase_on = !blink_phase_on;
    k_timer_start(&idle_blink_timer,
                  K_MSEC(blink_phase_on ? IDLE_BLINK_ON_MS : IDLE_BLINK_OFF_MS), K_NO_WAIT);
    /* Timer callbacks run in ISR context: never touch the SPI-attached LED here. */
    k_work_submit(&led_update_work);
}

/* --- Activity state listener --- */

static int activity_listener_cb(const zmk_event_t *eh) {
    activity = zmk_activity_get_state();

    if (activity == ZMK_ACTIVITY_IDLE) {
        blink_phase_on = true;
        k_timer_start(&idle_blink_timer, K_MSEC(IDLE_BLINK_ON_MS), K_NO_WAIT);
    } else {
        k_timer_stop(&idle_blink_timer);
    }

    led_request_update();
    return 0;
}

ZMK_LISTENER(activity_led_listener, activity_listener_cb);
ZMK_SUBSCRIPTION(activity_led_listener, zmk_activity_state_changed);

/* --- BLE connection listener --- */

static int ble_listener_cb(const zmk_event_t *eh) {
    /* The handler only mirrors the BLE state while active. */
    led_request_update();
    return 0;
}

ZMK_LISTENER(ble_led_listener, ble_listener_cb);
ZMK_SUBSCRIPTION(ble_led_listener, zmk_ble_active_profile_changed);

/* --- Init --- */

static int leds_init(void) {
    if (!device_is_ready(led_dev)) {
        LOG_ERR("LED device not ready");
        return -ENODEV;
    }

    activity = zmk_activity_get_state();
    led_request_update();

    return 0;
}

SYS_INIT(leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
