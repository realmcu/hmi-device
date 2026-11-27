/* Copyright (c) 2026 Realtek Semiconductor Corp.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "rtk_touch.h"

int main(void)
{
    printk("touch bound=%d timeout=%u ms\n",
           rtk_touch_get_default() != NULL, rtk_touch_get_timeout_ms());
    while (true)
    {
        rtk_touch_data_t data = rtk_touch_get_data();
        printk("touch press=%d x=%u y=%u\n", data.is_press, data.x, data.y);
        k_sleep(K_MSEC(100));
    }
    return 0;
}
