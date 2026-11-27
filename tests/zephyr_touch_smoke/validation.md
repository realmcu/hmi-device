# Touch/display decoupling validation

Validated on 2026-11-27 in the dashboard west workspace, with Zephyr 3.7.2,
Zephyr SDK 0.16.8 (ARM GCC 12.2.0), board `rtl87x3g_sample`.

| Check | Result |
|---|---|
| Host core compile/execute (`-Wall -Wextra -Werror`) | PASS |
| Isolated input Kconfig and single-chip config compatibility | PASS |
| CMake empty/core/display/mixed/key/four touch-only source graphs | PASS |
| Invalid multiple Zephyr touch controllers | Rejected as expected |
| Non-Zephyr core-only compile/link/execute without HAL/display | PASS |
| Non-Zephyr legacy/key HAL dependency retained | PASS |
| Zephyr CHSC6417 touch-only compile/link | PASS |
| Zephyr CHSC6413 touch-only compile/link | PASS |
| Zephyr CST816D touch-only compile/link | PASS |
| Zephyr CST820 touch-only compile/link | PASS |
| Zephyr core-only/no-controller compile/link | PASS |
| All five smoke configs/compile commands/ELF symbol isolation | PASS |
| Existing HoneyGUI display+touch+key compile/link/image packaging | PASS |
| Panel registry consistency | PASS (7 panels; existing legacy warnings) |

The smoke builds disable `REALTEK_DISPLAY`, `DISPLAY` and `REALTEK_HONEYGUI`.
`check_build.py` verified no display sources/includes in compile commands,
retained public touch API symbols, and exactly the selected controller and
binding (or neither for core-only) in the linked firmware.

CHSC6413 initially failed to link because its existing source referenced
`CONFIG_REALTEK_INPUT_INIT_PRIORITY`, which had no Kconfig definition. Input
Kconfig now owns this setting, default 60; the subsequent build passed. Chip
controller source files were relocated without changing their behavior.

No physical touch hardware was tested. Existing compiler warnings in vendor
HAL/driver and GUI code remain. Compile-smoke GPIO/I2C wiring is a test fixture,
not a board-specific production configuration. Older legacy driver ports and
SCons integration were not migrated to the new public core.
