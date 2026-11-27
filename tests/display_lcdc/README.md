# Shared LCDC regression tests

Run from the workspace root:

```powershell
python modules/device/tests/display_lcdc/run_tests.py
python modules/device/tests/display_lcdc/syntax_compile.py
python modules/device/tests/display_lcdc/init_failure.py
python modules/device/tests/run_input_tests.py
```

Host suite requires GCC, CMake, Ninja and Python `kconfiglib`. Syntax compilation
uses ARM GCC from Zephyr SDK 0.16.8 by default (`--gcc` overrides its path).
These tests do not establish physical 8762G/8773E hardware support: only 8773G has
a complete HAL/SDK in this workspace.

## Coverage

- `baseline.json` captured pre-refactor source hashes and public function names.
  Board files, panel C files and public SDI/RGB/display headers must retain their
  bytes; renamed controllers must retain their public function sets. The shared
  sources must not call G clock/Pad APIs or include `rtl876x_*` headers directly.
  Clock enum extensions are intentional and excluded from byte equality; host
  port tests assert the original 200M=0/280M=1 values.
- Seven real LCD register definition families (DBIC, DMA, DMA LL, eDPI, Handler,
  DBIB, RLSPI) must compare identically after stripping only comments, whitespace,
  SDK includes, base alias and the G-only clock enum. This is a source layout
  test, not a hardware capability claim.
- Kconfig tests cover G defaults, both G/E symbol aliases, E/2G opt-in, LCDC
  selection, display dependency, unsupported-SoC rejection and panel-manifest
  dependency on the shared option.
- Ten positive CMake graphs cover three SoCs' shared/nonshared selection, G
  default, G/E aliases and touch-only isolation. Assertions check exactly one
  selected SoC port, no old controller paths, no legacy LCD with shared enabled,
  unchanged board gate and no G DT wrapper on E/2G. Unsupported and multiple
  selected SoCs must fail configuration.
- `port_test.c` compiles/executes G, E and 2G against fake SDK calls under
  `-Wall -Wextra -Werror`. Tests assert supported clocks (G200/280, E200, 2G100),
  unsupported-clock rejection without clock side effects, RGB/QSPI Pad mode,
  pull, power/output/sleep, sentinel pins, HS-vs-dedicated dispatch, IRQ null,
  IRQ vector/priority3 and E/2G override return propagation, delays and SDI phase.
- `syntax_compile.py` performs nine ARM syntax compilations: shared SDI/RGB and
  public display core with each SoC's real LCD definitions plus minimal SDK type
  headers. The stubs assume an arbitrary base only for syntax; they do not
  validate actual SoC register base addresses and do not leak G clock enums into
  E/2G definitions.
- `init_failure.py` compiles/links/executes shared SDI/RGB early init against real
  E LCD definitions with host SDK types and trapping fake LCD functions. A clock
  callback returning -1 must stop init before LCD configuration/MMIO; repeated
  attempts must not be treated as initialized. Pad setup before clock failure is
  intentionally permitted to preserve the existing G initialization order.

## Limits

Fake HAL cannot validate actual PLL frequency, Pad routing, interrupt table,
ROM interfaces, cache/DMA constraints, electrical sleep state, or peripherals.
Syntax-only builds are **not real E/2G SDK builds**. Successful early failure tests
are not successful runtime transfer tests. Production G QSPI/RGB build and
on-device tests remain separately necessary.

Generated mocks and build files live in TemporaryDirectory. Tests never modify
production sources or boards. The pre-refactor baseline must not be regenerated
after migration. Existing input tests remain a separate suite.

Actual G firmware graph/symbol validation (requires a completed SDK build):

```powershell
python modules/device/tests/display_lcdc/validate_firmware.py `
  realtek-app/samples/honeygui/build_lcdc_shared --nm '<SDK>/arm-zephyr-eabi/bin/arm-zephyr-eabi-nm.exe'
python modules/device/tests/display_lcdc/validate_firmware.py `
  realtek-app/samples/honeygui/build_lcdc_shared_rgb --rgb --nm '<SDK>/arm-zephyr-eabi/bin/arm-zephyr-eabi-nm.exe'
```

The RGB build adds `ek9716.conf` through `EXTRA_CONF_FILE`; the sample's production
config remains unchanged. See `validation.md` for the recorded results and limits.
