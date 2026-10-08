# Totem Executive Edition

38-key ZMK firmware for the [Ergomech Totem Executive Edition](https://ergomech.store/shop/totem-executive-edition-522), based on [Ergomech’s ZMK-TOTEM firmware](https://github.com/ergomechstore/ZMK-TOTEMIST).

<details>
<summary>Keymap</summary>

![Totem keymap across Base, Number, Symbol, Navigation, Function, and System layers](images/totem-keymap-layers.svg)

</details>

## Layers

| Layer | Use |
| --- | --- |
| **Base** | QWERTY with home-row modifiers. Thumb holds access Navigation, Function, Symbol, and Number. |
| **Number** | Number pad, brackets, and number-row symbols. |
| **Symbol** | Punctuation and shifted symbols. |
| **Navigation** | Cursor and page movement, editing, and clipboard shortcuts. |
| **Function** | F-keys, media, brightness, search, emoji, screenshots, and screen lock shortcuts. |
| **System** | OS mode, Smart Shift, output, Bluetooth profiles, Studio, bootloader, and settings reset. |

Hold Space + Return to toggle Navigation, or Escape + Return to toggle System. On Navigation and System, the Escape thumb returns to Base.

## Key behaviors

- **OS mode** switches the active Bluetooth profile between macOS and Windows conventions. Each of the five profiles remembers its mode. In Windows mode, Control and GUI/Command swap; OS-specific shortcut keys send the matching shortcut for that system.
- **Smart Shift** toggles automatic sentence capitalization. **Caps Logic** toggles Caps Word on tap and sends Caps Lock on hold.
- **Meta** and **Hyper** are multi-modifier shortcuts. Their macOS and Windows modifier combinations are shown below the keymap.
- **Output toggle** switches between USB and Bluetooth. Bluetooth controls select profiles 0–4 or clear their pairings.
