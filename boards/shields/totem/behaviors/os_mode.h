#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zmk/behavior.h>

bool totem_os_windows_mode(void);
int totem_os_emit_keycode(uint32_t keycode,
                          struct zmk_behavior_binding_event event, bool pressed);
