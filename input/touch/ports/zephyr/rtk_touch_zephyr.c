/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "rtk_touch_zephyr.h"

#define TOUCH_DEV_NODE DT_NODELABEL(touch_device)
#define TOUCH_TIMEOUT_MS DT_PROP(TOUCH_DEV_NODE, gesture_release_timeout_ms)

static const struct device *touch_dev = DEVICE_DT_GET(TOUCH_DEV_NODE);

static rtk_touch_data_t touch_get_data(void)
{
    return get_raw_touch_data(touch_dev);
}

static const rtk_touch_t touch =
{
    .get_data = touch_get_data,
    .timeout_ms = TOUCH_TIMEOUT_MS,
};

static int touch_bind(void)
{
    if (!device_is_ready(touch_dev))
    {
        return -ENODEV;
    }

    rtk_touch_register(&touch);
    return 0;
}

SYS_INIT(touch_bind, APPLICATION, 0);
