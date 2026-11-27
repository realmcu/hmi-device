/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * EK9716 800x480 RGB/eDPI panel data.
 */

#include "rtk_rgb.h"

static const T_RTK_RGB_PANEL ek9716_800_480_rgb =
{
    .name = "ek9716-800x480-rgb",
    .width = 800,
    .height = 480,
    .input_bits = 16,
    .output_bits = 24,
    .clock_div = 8,
    .hsa = 48,
    .hbp = 40,
    .hfp = 40,
    .vsa = 1,
    .vbp = 31,
    .vfp = 13,
    .hsync_polarity = false,
    .vsync_polarity = false,
    .de_polarity = true,
    .reset_pre_ms = 20,
    .reset_low_ms = 20,
    .reset_post_ms = 20,
};

int rtk_rgb_new_panel_ek9716(const T_RTK_RGB_BUS *bus, rtk_rgb_t **out)
{
    return rtk_rgb_new_panel(&ek9716_800_480_rgb, bus, out);
}
