# Input regression tests

Run the SDK-independent host regression suite from the workspace root:

```powershell
python modules/device/tests/run_input_tests.py
```

Requirements: Python with `kconfiglib`, host GCC, CMake and Ninja on PATH.
Use `--gcc <compiler>` or `--cmake <executable>` to override tool locations.
Generated executables, mock projects and build directories live in a temporary
directory and are removed when the suite exits; source files are not modified.

Coverage:

- Compile and execute the touch core with warnings treated as errors. Assert
  initial zero/released fallback, registration, callback data fields, callback
  invocation, replacing the default device, missing callback, timeout and
  unregistering with NULL.
- Parse the device Kconfig entry independently of the display Kconfig. Mock
  external GUI/display/GPIO/I2C symbols and assert GUI-only core default,
  explicit disable, four controllers selecting core/GPIO/I2C, optional-choice
  mutual exclusion (loading existing single-driver `.conf` fragments), input
  initialization priority defaults/range, legacy core selection, key prerequisites, module entry
  registration and absence of old input definitions in display Kconfig.
- Configure a CMake harness which mocks `zephyr_library_named` and display/GUI
  targets, then inspect source/link/include target properties. Exercise empty,
  core-only, display-only with touch explicitly disabled, mixed, key-only and
  four controller touch-only configurations, plus rejection of an invalid
  multi-controller configuration. The display target is deliberately
  present even when display is disabled to detect accidental target-existence
  coupling.
- Build and execute a non-Zephyr core-only CMake target without supplying any
  display or HAL targets. Check that non-Zephyr legacy-touch, mixed and key
  graphs retain `halLib` (hardware-source compilation is not attempted).
  Reject each `_ZEPHYR` controller in a non-Zephyr CMake configuration.

The CMake harness does **not** compile Zephyr/HAL controller sources, evaluate
real device trees or link firmware. It tests build-graph isolation, not board or
hardware correctness. Real Zephyr compile/link and on-device tests remain
necessary. Kconfig choice tests load a fragment containing only the individual
controller's `CONFIG_...=y` setting, verifying compatibility with existing
application configurations without an explicit choice setting.

## Real Zephyr compile/link smoke test

`zephyr_touch_smoke/` is a minimal application without display or HoneyGUI. Build
with `rtl87x3g_sample`; select one of `chsc6417`, `chsc6413`, `cst816d`, `cst820`:

```powershell
west build -b rtl87x3g_sample modules/device/tests/zephyr_touch_smoke `
  -d modules/device/tests/zephyr_touch_smoke/build/chsc6417 -- `
  '-DEXTRA_CONF_FILE=chsc6417.conf' '-DDTC_OVERLAY_FILE=chsc6417.overlay' `
  '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
python modules/device/tests/zephyr_touch_smoke/check_build.py `
  modules/device/tests/zephyr_touch_smoke/build/chsc6417 `
  --nm '<SDK>/arm-zephyr-eabi/bin/arm-zephyr-eabi-nm.exe'
```

Use matching config/overlay names for the other chips. For a core-only fallback
build omit both extra config and overlay arguments. Set the usual Zephyr SDK
and toolchain environment variables for your installation.

`check_build.py` checks disabled display/GUI config, no display source/include
paths in compile commands, public API symbols in the ELF, and a retained binding
when a chip is selected. It requires a completed build. Board wiring is only a
compile-test fixture; no hardware correctness is implied. The smoke app omits
production image packaging and is not a replacement for application flashing.
