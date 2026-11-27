# Shared LCDC validation record

Validated 2026-11-27. Boards were deliberately not migrated.

| Verification | Result |
|---|---|
| Three SoC port default/IRQ-hook compile and host execution | PASS |
| Shared public controller function sets preserved | PASS |
| Existing board and panel source fingerprints unchanged | PASS |
| Seven LCD register-definition families normalized across SoCs | PASS |
| Shared/legacy/default/alias/input-only CMake graph isolation | PASS |
| Multiple/unsupported shared SoC selection rejected | PASS |
| SDI/RGB/facade ARM syntax with each SoC real LCD defs + SDK type stubs | PASS (9 combinations) |
| Controller clock failure stops initialization before LCD config/MMIO | PASS (host trapping fake HAL) |
| Existing input host suite | PASS |
| Panel registry consistency | PASS (7 panels, existing legacy warnings) |
| Actual 8773G HoneyGUI SH8601Z 410x502 build/link/image packaging | PASS (427/427, exit 0) |
| Actual 8773G HoneyGUI EK9716 RGB build/link/image packaging | PASS (427/427, exit 0) |
| Actual SDI/RGB compile graph and ELF shared-controller/SoC-port symbols | PASS |

Actual SDK builds used Zephyr 3.7.2, Zephyr SDK 0.16.8, ARM GCC 12.2.0,
`rtl87x3g_sample`. Outputs are in the existing HoneyGUI sample's ignored
`build_lcdc_shared/` and `build_lcdc_shared_rgb/` directories. The sample's
production config and boards were not changed by this refactor.

8762G and 8773E adapters are implemented, but the complete matching SDK/HAL,
board wiring and interrupt tables are absent from this workspace. Fake SDK
contracts and real LCD register-header syntax tests do not validate real base
addresses, frequencies, electrical behavior, DMA accessibility/cache handling,
IRQ delivery, or complete firmware linkage on those chips. No E/2G full-SDK or
physical-hardware pass is claimed. Their integrations must supply the real SDK
and bus data, and confirm or override the documented IRQ contract.
