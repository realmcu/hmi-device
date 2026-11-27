# Input Devices

Input-device configuration is owned by this module (`device/Kconfig`), not by
`display/Kconfig`. Existing `CONFIG_REALTEK_TOUCH_*` and `CONFIG_REALTEK_KEY_BUTTON_*`
symbol names remain valid.

```text
input/
├── Kconfig
├── touch/
│   ├── Kconfig
│   ├── core/                 # OS/SoC-independent data and registration API
│   ├── controller/zephyr/    # CHSC6417, CHSC6413, CST816D, CST820 drivers
│   └── ports/zephyr/         # shared driver contract and DT/default binding
├── keyboard/                 # reserved
├── key/                      # reserved
└── knob/                     # reserved
```

## Touch layers

- `core/rtk_touch.h` exposes `rtk_touch_data_t`, `rtk_touch_register()`,
  `rtk_touch_get_default()`, `rtk_touch_get_data()` and `rtk_touch_get_timeout_ms()`.
  It has no Zephyr, display, or SoC dependency. A missing/unregistered driver
  returns released/zero data and zero timeout.
- `controller/zephyr/` contains the existing chip protocol, reset, interrupt,
  workqueue and release-timer implementations. These drivers still use Zephyr
  directly; they are **not** advertised as OS-independent chip implementations.
  I2C/GPIO are provided by Zephyr, so there is no artificial `_8773g` backend.
- `ports/zephyr/rtk_touch_zephyr.h` defines a shared contract. `TOUCH_DATA` is a
  compatibility alias for `rtk_touch_data_t`; chip-specific headers are retained
  for existing applications. The binding includes only the shared contract,
  not a selected chip header.
- `ports/zephyr/rtk_touch_zephyr.c` binds `DT_NODELABEL(touch_device)` after device
  initialization. Board bus/pins/address/power/timeout remain in devicetree.

## Independent configuration

Core only (including a custom registered provider):

```conf
CONFIG_REALTEK_TOUCH=y
CONFIG_REALTEK_DISPLAY=n
```

Zephyr CHSC6417 (with an enabled, correctly wired `touch_device` DT node):

```conf
CONFIG_REALTEK_TOUCH_CHSC6417_ZEPHYR=y
CONFIG_REALTEK_DISPLAY=n
```

The controller selects the core, I2C and GPIO; enable the board's corresponding
bus/pinctrl/interrupt nodes. `REALTEK_INPUT_INIT_PRIORITY` defaults to 60 for
CHSC6413, which previously referenced this setting without a definition; keep
it after the board's I2C/GPIO initialization priorities. The optional Zephyr controller choice permits at
most one chip because the current raw-data API uses a shared symbol. CMake also
rejects multiple controllers when driven without Kconfig. `_ZEPHYR` drivers are
built only in a Zephyr environment.

`REALTEK_TOUCH` defaults to enabled for HoneyGUI's existing no-touch fallback,
not for display generally. A display-only application can leave it disabled;
input-only builds do not create or link the display hardware library, panel
registry, display adapter, or GUI acceleration. The shared
`realtek_device_driver_library` target remains compatible with current apps.

Legacy SoC input drivers stay under `legacy/<soc>/`. Their hardware/OS and
application-board headers are still required; this change does not port old
vendor drivers or remove their application-specific dependencies. Legacy touch
drivers have not gained a `rtk_touch_register()` bridge: applications using them
still use their existing legacy APIs. The new core is supported by CMake;
existing SCons entry points remain legacy-only.

Applications must fetch/register the `device` Zephyr module in their manifest,
not just `display`: input symbols are no longer supplied by the display module.
The active dashboard workspace already includes both modules.

## Validation

From the workspace root:

```bash
python modules/device/tests/run_input_tests.py
west build -b rtl87x3g_sample modules/device/tests/zephyr_touch_smoke \
  -d modules/device/tests/zephyr_touch_smoke/build/chsc6417 -- \
  -DEXTRA_CONF_FILE=chsc6417.conf -DDTC_OVERLAY_FILE=chsc6417.overlay
```

The smoke application consumes the public API without display/HoneyGUI. Its
wiring is a compile-test fixture, not a promise of compatibility with every
board. Validate readiness, power/reset, coordinates and release timeout on the
actual hardware. The smoke app deliberately omits production image packaging.
