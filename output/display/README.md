# Shared LCDC display devices

```text
display/
├── core/                   # public types, facade and transfer synchronization
├── controller/lcdc/        # shared SDI/DBIC and RGB/eDPI/DMA implementations
├── ports/                  # 8773G / 8773E / 8762G clock, Pad, IRQ and delays
├── panel/                  # shared panel command tables and RGB timings
├── boards/                 # existing board wiring (unchanged)
└── registry/               # existing panel registration
```

The three SDK LCDC register models share DMA/DBIC/eDPI structures and bitfields.
The old `rtk_sdi_8773g.c` and `rtk_rgb_8773g.c` therefore became
`controller/lcdc/rtk_sdi_lcdc.c` and `rtk_rgb_lcdc.c`. Their public API and pixel
transfer algorithms are preserved. Clock, Pad, electrical sleep state, delays,
IRQ installation and the SDI phase-shift default now go through
`ports/rtk_display_soc.h`. Exactly one port is selected per build.

## Selecting the shared path

```conf
CONFIG_REALTEK_DISPLAY=y
CONFIG_REALTEK_DISPLAY_LCDC_SHARED=y
```

`REALTEK_DISPLAY_LCDC_SHARED` defaults to enabled on 8773G, preserving existing
applications. On 8773E/8762G it is explicit opt-in and selects LCDC hardware.
Legacy LCD sources are excluded when shared controllers are enabled, preventing
duplicate panel/helper symbols. Legacy touch/key sources remain independent.
The existing panel-manifest mode is available on all shared SoCs:

```conf
CONFIG_REALTEK_DISPLAY_USE_PANEL_MANIFEST=y
```

The registry's historical `RTK_8773G_*` CMake variable names are retained for
compatibility, not as a statement that the panel data is G-only.

## Boards are not migrated

No file under `boards/` was changed. Only the existing 8773G board descriptors
and Zephyr display wrapper are auto-built. E/2G applications must supply their
own `T_RTK_SDI_BUS` or `T_RTK_RGB_BUS` (pins, group, power, reset, bus clock and
DBIC divider), then create a panel/controller and the display facade using the
existing APIs. Do not use 8773G pin or clock descriptors on another chip.

Supported bus clock selections:

| SoC | Clock selections | Source |
|---|---|---|
| 8773G | `RTK_DISPLAY_CLK_200M`, `RTK_DISPLAY_CLK_280M` | existing clock-hook integration |
| 8773E | `RTK_DISPLAY_CLK_200M` | legacy PLL1/div1 configuration |
| 8762G | `RTK_DISPLAY_CLK_100M` | legacy PLL1 100 MHz register sequence |

The original 200/280 enum values are unchanged. 40/160 clock enum values are
reserved; none of the current ports accepts them. Unsupported clocks cause
controller initialization to return failure rather than silently selecting a
different frequency. RGB `clock_div` remains panel data; confirm it produces the
required pixel clock on the selected SoC. Pad routing and SDI phase shift are
also port-specific: G/E use high-speed mux and phase 2; 2G uses dedicated pads
and phase 0. See `ports/README.md` for exact SDK assumptions and IRQ overrides.

## SDK requirements and validation boundaries

The current workspace contains the complete 8773G SDK/HAL only. E/2G ports are
implemented from the legacy source and require the corresponding real SDK/HAL,
address map, IRQ table and OSIF (`os_sync.h`) from the integration. Their default
IRQ contract installs a RAM vector and enables NVIC; an SDK with different
installation semantics must override `RTK_DISPLAY_SOC_IRQ_INSTALL(handler)`.
Missing SDK symbols are errors, not fake successful stubs.

- Host port tests compile and execute all three adapters against explicit fake
  SDK contracts; they test call sequences and error paths, not hardware.
- ARM syntax tests compile shared sources with each real LCDC register-definition
  set plus SDK type stubs. This checks shared API/register-layout compatibility,
  not real base addresses or complete E/2G SDK linkage.
- Actual 8773G HoneyGUI 410x502 compile/link/image packaging and RGB builds
  validate the existing integration after migration.
- No E/2G full-SDK firmware or hardware test is claimed; these require external
  SDKs and board data. SCons remains on the legacy path.

From the workspace root:

```powershell
python modules/device/tests/display_lcdc/run_tests.py
python modules/device/tests/display_lcdc/syntax_compile.py
python modules/device/tests/display_lcdc/init_failure.py
python modules/device/tests/run_input_tests.py
python modules/device/output/display/registry/panel_registry.py --check
```
