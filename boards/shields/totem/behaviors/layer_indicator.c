#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

static uint8_t current_layer(void) { return zmk_keymap_highest_layer_active(); }

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zephyr/bluetooth/gatt.h>

#define TOTEM_LAYER_UUID(suffix)                                                                    \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(suffix, 0x58aa, 0x4562, 0xa224, 0x204bad21c138))

static ssize_t read_layer(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                          uint16_t len, uint16_t offset) {
    uint8_t layer = current_layer();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &layer, sizeof(layer));
}

BT_GATT_SERVICE_DEFINE(
    totem_layer, BT_GATT_PRIMARY_SERVICE(TOTEM_LAYER_UUID(0xebb4f000)),
    BT_GATT_CHARACTERISTIC(TOTEM_LAYER_UUID(0xebb4f001), BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, read_layer, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT));
#endif

#if IS_ENABLED(CONFIG_ZMK_USB)
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) && \
    CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS == 2
#include <zephyr/sys/atomic.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/position_state_changed.h>

#define TOTEM_USB_BATTERY 1
#define BATTERY_UNKNOWN 0xFF

// Store source and charge plus one, so zero represents an unread value.
static atomic_t battery_sources[2];
static atomic_t battery_levels[2];
static uint8_t usb_read_battery[2];

static int battery_report_listener(const zmk_event_t *event) {
    const struct zmk_peripheral_battery_state_changed *battery =
        as_zmk_peripheral_battery_state_changed(event);
    if (battery && battery->source < ARRAY_SIZE(battery_levels) && battery->state_of_charge <= 100) {
        atomic_set(&battery_levels[battery->source], battery->state_of_charge + 1);
    }

    const struct zmk_position_state_changed *key = as_zmk_position_state_changed(event);
    if (key && key->state && key->source < ARRAY_SIZE(battery_sources) && key->position < 38) {
        // Match the physical right half in the Totem's 38-key layout.
        bool right = (key->position >= 5 && key->position <= 9) ||
                     (key->position >= 15 && key->position <= 19) ||
                     (key->position >= 26 && key->position <= 31) || key->position >= 35;
        atomic_set(&battery_sources[right], key->source + 1);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(battery_report, battery_report_listener);
ZMK_SUBSCRIPTION(battery_report, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(battery_report, zmk_position_state_changed);
#endif

static const struct device *layer_hid;
static uint8_t usb_read_layer;
static uint8_t usb_input_layer;
static K_SEM_DEFINE(usb_idle, 1, 1);
static const uint8_t layer_descriptor[] = {
    0x06, 0x71, 0xFF, // Vendor usage page 0xFF71
    0x09, 0x01,       // Totem layer status
    0xA1, 0x01,       // Application collection
    0x09, 0x02,       // Layer index
    0x15, 0x00, 0x25, 0x06,
    0x75, 0x08, 0x95, 0x01,
    0x81, 0x02,       // One byte of input data
#if defined(TOTEM_USB_BATTERY)
    0x09, 0x03,       // Left half
    0x09, 0x04,       // Right half
    0x26, 0xFF, 0x00, // 0–100 percent or 0xFF for unknown
    0x95, 0x02,
    0xB1, 0x02,       // Two-byte feature report, separate from layer input
#endif
    0xC0,
};

static int usb_get_report(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
                         uint8_t **data) {
#if defined(TOTEM_USB_BATTERY)
    if (setup->wValue == 0x0300) {
        for (size_t side = 0; side < ARRAY_SIZE(usb_read_battery); side++) {
            atomic_val_t source = atomic_get(&battery_sources[side]);
            atomic_val_t charge = source ? atomic_get(&battery_levels[source - 1]) : 0;
            usb_read_battery[side] = charge ? charge - 1 : BATTERY_UNKNOWN;
        }
        *len = sizeof(usb_read_battery);
        *data = usb_read_battery;
        return 0;
    }
#endif
    if (setup->wValue != 0x0100) {
        return -ENOTSUP;
    }
    usb_read_layer = current_layer();
    *len = sizeof(usb_read_layer);
    *data = &usb_read_layer;
    return 0;
}

static void usb_layer_ready(const struct device *dev) { k_sem_give(&usb_idle); }

static const struct hid_ops layer_ops = {
    .get_report = usb_get_report,
    .int_in_ready = usb_layer_ready,
};

static void send_usb_layer(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(usb_layer_work, send_usb_layer);

static void send_usb_layer(struct k_work *work) {
    if (!layer_hid || !zmk_usb_is_hid_ready() || zmk_usb_get_status() == USB_DC_SUSPEND) {
        return;
    }

    if (k_sem_take(&usb_idle, K_NO_WAIT) != 0) {
        k_work_reschedule(&usb_layer_work, K_MSEC(10));
        return;
    }

    // The nRF USB controller reads this buffer after the write returns.
    usb_input_layer = current_layer();
    int err = hid_int_ep_write(layer_hid, &usb_input_layer, sizeof(usb_input_layer), NULL);
    if (err) {
        k_sem_give(&usb_idle);
        if (err == -EAGAIN) {
            k_work_reschedule(&usb_layer_work, K_MSEC(10));
        }
    }
}

static int layer_usb_init(void) {
    layer_hid = device_get_binding("HID_1");
    if (!layer_hid) {
        return -ENODEV;
    }
    usb_hid_register_device(layer_hid, layer_descriptor, sizeof(layer_descriptor), &layer_ops);
    return usb_hid_init(layer_hid);
}

SYS_INIT(layer_usb_init, APPLICATION, CONFIG_ZMK_USB_HID_INIT_PRIORITY);
#endif

static void send_layer(struct k_work *work) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    uint8_t layer = current_layer();
    bt_gatt_notify(NULL, &totem_layer.attrs[2], &layer, sizeof(layer));
#endif
#if IS_ENABLED(CONFIG_ZMK_USB)
    k_work_reschedule(&usb_layer_work, K_NO_WAIT);
#endif
}

static K_WORK_DEFINE(layer_work, send_layer);

static int layer_indicator_listener(const zmk_event_t *event) {
#if IS_ENABLED(CONFIG_ZMK_USB)
    if (as_zmk_usb_conn_state_changed(event)) {
        if (!zmk_usb_is_hid_ready()) {
            k_sem_give(&usb_idle);
        }
        k_work_reschedule(&usb_layer_work, K_NO_WAIT);
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif
    k_work_submit(&layer_work);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_indicator, layer_indicator_listener);
ZMK_SUBSCRIPTION(layer_indicator, zmk_layer_state_changed);
#if IS_ENABLED(CONFIG_ZMK_USB)
ZMK_SUBSCRIPTION(layer_indicator, zmk_usb_conn_state_changed);
#endif
