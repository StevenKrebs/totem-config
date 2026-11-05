# Central dongle firmware

This branch implements dongle-central firmware. Hardware pairing and runtime behavior require verification when the dongle arrives.

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

## Implemented configuration

- `build.yaml` builds `totem_left` and `totem_right` as peripherals, `totem_dongle` as central with USB Studio support, and XIAO `settings_reset` firmware.
- `config/west.yml` pins the existing Totem module to `6b76bfbc5356cb5d93fc2d64acf44608bfc50187`, which already supplies the matching dongle shield and physical layout. No copied shield is needed.
- `config/totem.conf` keeps radio power and battery reporting on both halves.
- `config/totem_dongle.conf` configures two peripheral connections and seven connection/bond slots, preserving five host Bluetooth profiles. It keeps the USB-powered dongle awake and enables split battery fetching/proxy reporting.
- ZMK remains on v0.3 with `seeeduino_xiao_ble` board naming. The newer vendor example uses `xiao_ble//zmk`; those names are not mixed here.
- The existing Gaming keymap, Executive Edition geometry, OS mode, and smart behaviors are retained. Processing and saved central settings now live on the dongle.

## Firmware outputs

The GitHub Actions build produces separate UF2 files for `totem_left`, `totem_right`, `totem_dongle`, and `settings_reset`, all for the XIAO nRF52840. Use the matching file for each device; the same settings-reset image applies to all three XIAO controllers.

## Migration and rollback

After successful builds and once the dongle arrives, flash settings-reset firmware on **all three devices**, then their matching firmware. Resetting settings removes existing pairing information and saved settings. Verify both halves, every physical key, Gaming/System entry and exit, USB host output, Studio, and battery reporting.

Both halves will depend on the dongle after conversion; they will no longer connect directly to the host. To roll back, build the two-half configuration from `main`, reset settings on all devices again, flash the original roles, and re-pair. Preserve the current working firmware before migration.
