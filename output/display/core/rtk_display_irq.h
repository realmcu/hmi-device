/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTK_DISPLAY_IRQ_H
#define RTK_DISPLAY_IRQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTK_DISPLAY_WAIT_FOREVER 0xFFFFFFFFu

int rtk_display_irq_init(void);
int rtk_display_irq_prepare(void);
int rtk_display_irq_wait(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* RTK_DISPLAY_IRQ_H */
