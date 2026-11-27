/* SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <stdio.h>
#include "rtk_touch.h"

static unsigned calls;
static rtk_touch_data_t read_touch(void)
{
    ++calls;
    return (rtk_touch_data_t)
    {
        .x = 123, .y = 456, .x_start = 12,
         .y_start = 34, .timestamp_ms_start = 100,
          .timestamp_ms_pressing = 200, .count_pressing = 3, .is_press = true
    };
}

static void assert_released(void)
{
    rtk_touch_data_t data = rtk_touch_get_data();
    assert(data.x == 0 && data.y == 0 && data.x_start == 0 && data.y_start == 0);
    assert(data.timestamp_ms_start == 0 && data.timestamp_ms_pressing == 0);
    assert(data.count_pressing == 0 && !data.is_press);
}

int main(void)
{
    static const rtk_touch_t first = {.get_data = read_touch, .timeout_ms = 37};
    static const rtk_touch_t second = {.get_data = read_touch, .timeout_ms = 99};
    static const rtk_touch_t no_callback = {.get_data = NULL, .timeout_ms = 42};
    assert(rtk_touch_get_default() == NULL);
    assert(rtk_touch_get_timeout_ms() == 0);
    assert_released();
    rtk_touch_register(&first);
    assert(rtk_touch_get_default() == &first);
    assert(rtk_touch_get_timeout_ms() == 37);
    rtk_touch_data_t data = rtk_touch_get_data();
    assert(calls == 1 && data.x == 123 && data.y == 456);
    assert(data.x_start == 12 && data.y_start == 34);
    assert(data.timestamp_ms_start == 100 && data.timestamp_ms_pressing == 200);
    assert(data.count_pressing == 3 && data.is_press);
    rtk_touch_register(&second);
    assert(rtk_touch_get_default() == &second && rtk_touch_get_timeout_ms() == 99);
    (void)rtk_touch_get_data();
    assert(calls == 2);
    rtk_touch_register(&no_callback);
    assert_released();
    assert(calls == 2 && rtk_touch_get_timeout_ms() == 42);
    rtk_touch_register(NULL);
    assert(rtk_touch_get_default() == NULL && rtk_touch_get_timeout_ms() == 0);
    assert_released();
    puts("PASS touch core registration, callback, replacement, fallback, timeout");
    return 0;
}
