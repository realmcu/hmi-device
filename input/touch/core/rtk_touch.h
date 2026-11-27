/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTK_TOUCH_H
#define RTK_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t x_start;
    uint16_t y_start;
    uint32_t timestamp_ms_start;
    uint32_t timestamp_ms_pressing;
    uint16_t count_pressing;
    bool is_press;
} rtk_touch_data_t;

typedef struct
{
    rtk_touch_data_t (*get_data)(void);
    uint32_t timeout_ms;
} rtk_touch_t;

/* Register before consumers start. The provider must outlive its registration;
 * NULL unregisters it. Synchronization with readers is the caller's responsibility.
 */
void rtk_touch_register(const rtk_touch_t *touch);
const rtk_touch_t *rtk_touch_get_default(void);
rtk_touch_data_t rtk_touch_get_data(void);
uint32_t rtk_touch_get_timeout_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* RTK_TOUCH_H */
