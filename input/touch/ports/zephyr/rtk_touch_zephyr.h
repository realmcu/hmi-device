/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef RTK_TOUCH_ZEPHYR_H
#define RTK_TOUCH_ZEPHYR_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include "rtk_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compatibility name retained for existing chip-driver consumers. */
typedef rtk_touch_data_t TOUCH_DATA;

/* One controller is selected per build; dev identifies its DT instance. */
TOUCH_DATA get_raw_touch_data(const struct device *dev);

#ifdef __cplusplus
}
#endif
#endif /* RTK_TOUCH_ZEPHYR_H */
