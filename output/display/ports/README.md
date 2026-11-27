# LCDC SoC adapters

The shared controller uses `rtk_display_soc.h`; exactly one backend is selected
by the display ports CMake file. No pin numbers or board wiring are defined here.
`RTK_DISPLAY_PIN_NONE` is a no-op for all pad functions.

| Backend | Accepted bus clock | Pad routing | SDI phase shift |
|---|---|---|---|
| RTL8773G | 200 or 280 MHz | HS_Func0 / FROM_CORE_DOMAIN | 2 |
| RTL8773E | 200 MHz only | HS_Func0 / FROM_CORE_DOMAIN | 2 |
| RTL8762G | 100 MHz only | Pad_Dedicated_Config(ENABLE) | 0 |

Unknown or unsupported clock values return -1 before clock-register writes.
No backend quietly maps an unsupported frequency to another source. The enum
may advertise 40/160 MHz for future integrations, but the current 2G adapter
rejects them until full clock/divider sequences are verified. In particular,
existing 200/280 MHz G board descriptors cannot be reused unchanged on 2G.

## Evidence and limits

- G preserves the original `LCDC_Clock_Sel()` calls, including clock pre/post
  hooks and their existing return-value treatment, as well as original Pad
  modes, pull configuration and sleep routing. IRQ uses the established
  `DISPLAY_VECTORn` / `Display_IRQn`, RAM vector update and NVIC priority 3.
- E's 200 MHz PLL1/div1 setup is documented explicitly by
  `legacy/8773E/lcd/lcd_st7265_800480_rgb.c:250`; its QSPI drivers use the same
  source/divider setup. No assumption is made that its PLL2 equals G's 280 MHz.
  The Pad setup follows E's RGB and SH8601Z QSPI examples.
- 2G copies the complete PLL1 100 MHz source/divider sequence from
  `legacy/8762G/lcd/EK9716_800480_rgb.c:148-161`. Its source comments mention
  PLL2=160 MHz and XTAL=40 MHz, but do not prove complete matching divider/setup
  sequences; those clocks remain rejected. The same RGB driver demonstrates
  `Pad_Dedicated_Config()` and explicitly declares its prototype. The 2G
  CO5300 QSPI/TE path also uses PINMUX mode before dedicated-pad enable, so the
  2G backend uses PINMUX for both buses rather than copying E/G's DBIC SW mode.
- 2G sleep uses the software-pad low/pulldown/output-disabled state. It does
  not invent a dedicated-pad-disable operation: none occurs in the available
  legacy LCD source. Confirm board electrical behavior before deployment.
- E legacy source includes the rtl876x NVIC/vector-table SDK family but does
  not show a display ISR installation. 2G legacy source names `Display_IRQn`
  and includes `rtl_nvic.h`, but does not show the RAM vector installation.
  The E/2G default installation code therefore declares an **SDK contract**:
  `vector_table.h`, `RamVectorTableUpdate`, `DISPLAY_VECTORn`, `Display_IRQn`
  and `NVIC_Init` must exist and match the actual chip. Missing symbols must
  fail compilation rather than yield a fake successful/no-op installation.

E/2G SDK integrations may override `RTK_DISPLAY_SOC_IRQ_VECTOR` and
`RTK_DISPLAY_SOC_IRQ_NUMBER`, or define `RTK_DISPLAY_SOC_IRQ_INSTALL(handler)`
as an int-returning hook to replace the entire installation path (0 success,
nonzero failure). Supply the hook declaration through an integration/forced
include header. With the whole hook override, no default NVIC or vector-table
header is included. NULL handlers are always rejected.

The checkout contains a complete 8773G HAL only. E and 2G require their real
SDKs, clock source initialization, IRQ-table confirmation, and compile/link
plus hardware testing. Mock-contract compilation can test adapter call
sequences, but is not proof of E/2G hardware support or clock accuracy.
