#define DT_DRV_COMPAT zmk_behavior_smart_modifier

#include <drivers/behavior.h>
#include <zephyr/device.h>

#include <zmk/behavior.h>

#include "os_mode.h"

struct smart_modifier_config {
    uint32_t mac_key;
    uint32_t windows_key;
};

#define SMART_MODIFIER_POSITION_COUNT 64
static uint32_t active_keycodes[SMART_MODIFIER_POSITION_COUNT];

static int smart_modifier_invoke(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event, bool pressed) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct smart_modifier_config *config = dev->config;
    uint32_t keycode = totem_os_windows_mode() ? config->windows_key : config->mac_key;

    if (event.position < SMART_MODIFIER_POSITION_COUNT) {
        if (pressed) {
            active_keycodes[event.position] = keycode;
        } else if (active_keycodes[event.position] != 0) {
            keycode = active_keycodes[event.position];
            active_keycodes[event.position] = 0;
        }
    }

    return totem_os_emit_keycode(keycode, event, pressed);
}

static int smart_modifier_pressed(struct zmk_behavior_binding *binding,
                                  struct zmk_behavior_binding_event event) {
    return smart_modifier_invoke(binding, event, true);
}

static int smart_modifier_released(struct zmk_behavior_binding *binding,
                                   struct zmk_behavior_binding_event event) {
    return smart_modifier_invoke(binding, event, false);
}

static const struct behavior_driver_api smart_modifier_api = {
    .binding_pressed = smart_modifier_pressed,
    .binding_released = smart_modifier_released,
};

#define SMART_MODIFIER_INST(n)                                                     \
    static const struct smart_modifier_config smart_modifier_config_##n = {        \
        .mac_key = DT_INST_PROP(n, mac_key),                                       \
        .windows_key = DT_INST_PROP(n, windows_key),                               \
    };                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &smart_modifier_config_##n,       \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,      \
                            &smart_modifier_api);

DT_INST_FOREACH_STATUS_OKAY(SMART_MODIFIER_INST)
