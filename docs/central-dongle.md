# Central dongle preparation

Status: preparation only. This branch retains the existing two-half firmware and pairing roles. No dongle firmware has been built or flashed.

## Ordered hardware

The KeebSupply order confirmation dated 2026-11-01 lists one **Wireless Dongle**. The matching product identifies the controller as **Seeed Studio XIAO nRF52840**, SKU `nRF52840-dongle-kit`, with a printed case and USB-C cable:
https://keeb.supply/products/wireless-dongle

## Reference code

KeebSupply links this Totem example directly from the product:
https://github.com/Conor-Burns/zmk-config-totem-dongle

Its build matrix uses `xiao_ble//zmk` with `totem_left`, `totem_right`, `totem_dongle`, and `settings_reset`:
https://github.com/Conor-Burns/zmk-config-totem-dongle/blob/main/build.yaml

The authoritative integration guide is:
https://zmk.dev/docs/hardware-integration/dongle

## Implementation scope

1. Keep this repository's keymap, custom behaviors, and Executive Edition layout. Do not replace them with the example's Colemak-DH keymap.
2. Add a local dongle shield with mock kscan, central role, and **two** peripheral connections. Copy the exact matrix transform and physical layouts from the `victorlucachi/zmk-keyboards-totem` module used by this repository, preserving node names and removing any physical-layout kscan assignment on the dongle.
3. Convert both halves to peripherals and move Studio USB support to the dongle. Keep the shared keymap and custom behavior module available to the dongle. Check custom behavior compilation on peripheral and settings-reset builds.
4. Review `config/totem.conf` USB boot and central battery-fetch/proxy options for their new device roles; validate resolved Kconfig for all builds.
5. Build the dongle, both halves, and XIAO settings-reset firmware. Validate the dongle's layout in Studio and its custom OS mode and smart behaviors.

More than three files will be necessary for implementation: the shield needs its own overlay and Kconfig declarations, plus build matrix changes. A build-matrix-only change cannot supply the dongle's matrix transform or central role defaults.

## Build matrix sketch for this repository's ZMK v0.3

This is an implementation example, not an active build definition. `totem_dongle` must first be defined locally as described above.

```yaml
include:
  - board: seeeduino_xiao_ble
    shield: totem_left
    cmake-args: -DCONFIG_ZMK_SPLIT=y -DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
  - board: seeeduino_xiao_ble
    shield: totem_right
    cmake-args: -DCONFIG_ZMK_SPLIT=y -DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
  - board: seeeduino_xiao_ble
    shield: totem_dongle
    snippet: studio-rpc-usb-uart
    cmake-args: -DCONFIG_ZMK_STUDIO=y
  - board: seeeduino_xiao_ble
    shield: settings_reset
```

The example repository uses newer `xiao_ble//zmk` board naming. This repository pins ZMK **v0.3** and currently uses `seeeduino_xiao_ble`; do not mix board names across versions or upgrade ZMK as part of the keymap port. Confirm the dongle controller/bootloader when it arrives before flashing.

For the dongle shield, set `ZMK_SPLIT=y`, `ZMK_SPLIT_ROLE_CENTRAL=y`, and `ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2`. If retaining five host Bluetooth profiles, set both `BT_MAX_CONN` and `BT_MAX_PAIRED` to **7** (two peripherals plus five profiles), as prescribed by the integration guide. Check these defaults against v0.3 before implementation.

## Migration and rollback

After successful builds and once the dongle arrives, flash settings-reset firmware on **all three devices**, then their matching firmware. Resetting settings removes existing pairing information and saved settings. Verify both halves, every physical key, Gaming/System entry and exit, USB host output, Studio, and battery reporting.

Both halves will depend on the dongle after conversion; they will no longer connect directly to the host. To roll back, build the two-half configuration from `main`, reset settings on all devices again, flash the original roles, and re-pair. Preserve the current working firmware before migration.
