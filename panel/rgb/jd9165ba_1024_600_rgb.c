/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * JD9165BA 1024x600 RGB/eDPI panel data.
 */

#include "rtk_rgb.h"

static const T_RTK_RGB_PANEL jd9165ba_1024_600_rgb =
{
    .name = "jd9165ba-1024x600-rgb",
    .width = 1024,
    .height = 600,
    .input_bits = 16,
    .output_bits = 24,
    .clock_div = 4,
    .hsa = 24,
    .hbp = 136,
    .hfp = 160,
    .vsa = 2,
    .vbp = 21,
    .vfp = 12,
    .hsync_polarity = false,
    .vsync_polarity = false,
    .de_polarity = true,
    .reset_pre_ms = 20,
    .reset_low_ms = 20,
    .reset_post_ms = 120,
};

int rtk_rgb_new_panel_jd9165ba(const T_RTK_RGB_BUS *bus, rtk_rgb_t **out)
{
    return rtk_rgb_new_panel(&jd9165ba_1024_600_rgb, bus, out);
}
