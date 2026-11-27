/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef RTK_DISPLAY_SOC_H
#define RTK_DISPLAY_SOC_H

#include <stdbool.h>
#include <stdint.h>
#include "rtk_display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configure an explicitly supported LCDC bus clock. Unknown clocks return -1;
 * no backend silently substitutes another frequency. Panel dividers remain
 * panel data. The selected SDK must supply/enable its documented PLL sources.
 */
int rtk_display_soc_clock_config(T_RTK_DISPLAY_CLK clock);
/* rgb selects DPI rather than DBIC/QSPI/TE routing. E/G use PINMUX for DPI
 * and SW mode for DBIC; 2G uses PINMUX + dedicated routing for either bus.
 * Bus outputs are disabled and their initial output latch is high.
 */
void rtk_display_soc_pad_bus(uint32_t pin, bool rgb, bool pull_up);
void rtk_display_soc_pad_output(uint32_t pin, bool high);
void rtk_display_soc_pad_sleep(uint32_t pin);
/* Install and enable a display interrupt at priority 3. The shared IRQ layer
 * owns pending/mask handling. Legacy SDK integrations may define
 * RTK_DISPLAY_SOC_IRQ_INSTALL(handler) as an int-returning install operation;
 * the hook declaration must be made available by the integration's headers.
 */
int rtk_display_soc_irq_install(void (*handler)(void));
void rtk_display_soc_delay_ms(uint32_t delay);
void rtk_display_soc_delay_us(uint32_t delay);
uint8_t rtk_display_soc_sdi_phase_shift(void);

#ifdef __cplusplus
}
#endif
#endif /* RTK_DISPLAY_SOC_H */
