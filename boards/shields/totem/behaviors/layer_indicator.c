#include <zephyr/kernel.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

static void send_layer_indicator(struct k_work *work) {
    zmk_keymap_layer_index_t layer = zmk_keymap_highest_layer_active();
    if (layer > 6) {
        return;
    }

    // Send directly so the signal doesn't affect key repeat or Smart Shift.
    zmk_key_t key = HID_USAGE_KEY_KEYBOARD_F13 + layer;
    zmk_hid_keyboard_press(key);
    zmk_endpoints_send_report(HID_USAGE_KEY);
    zmk_hid_keyboard_release(key);
    zmk_endpoints_send_report(HID_USAGE_KEY);
}

static K_WORK_DEFINE(layer_indicator_work, send_layer_indicator);
static K_WORK_DELAYABLE_DEFINE(layer_indicator_sync, send_layer_indicator);

static int layer_indicator_listener(const zmk_event_t *event) {
    if (as_zmk_layer_state_changed(event)) {
        k_work_submit(&layer_indicator_work);
    } else {
        // Give the host time to resume HID input after connecting.
        k_work_reschedule(&layer_indicator_sync, K_SECONDS(1));
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_indicator, layer_indicator_listener);
ZMK_SUBSCRIPTION(layer_indicator, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(layer_indicator, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(layer_indicator, zmk_ble_active_profile_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_USB)
ZMK_SUBSCRIPTION(layer_indicator, zmk_usb_conn_state_changed);
#endif
