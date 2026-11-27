/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include "rtk_touch.h"

static const rtk_touch_t *default_touch;

void rtk_touch_register(const rtk_touch_t *touch)
{
    default_touch = touch;
}

const rtk_touch_t *rtk_touch_get_default(void)
{
    return default_touch;
}

rtk_touch_data_t rtk_touch_get_data(void)
{
    if (default_touch == NULL || default_touch->get_data == NULL)
    {
        return (rtk_touch_data_t) {0};
    }

    return default_touch->get_data();
}

uint32_t rtk_touch_get_timeout_ms(void)
{
    return default_touch == NULL ? 0U : default_touch->timeout_ms;
}
