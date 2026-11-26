/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "board_display.h"

#include "rtl876x_pinmux.h"

const T_RTK_SDI_BUS board_sdi_sh8601_bus =
{
    .clock = RTK_DISPLAY_CLK_280M,
    .pin_group = 1,
    .cs = P9_2,
    .clk = P9_4,
    .d0 = P9_3,
    .d1 = P9_1,
    .d2 = P9_0,
    .d3 = P9_5,
    .te = P2_2,
    .rst = P4_4,
    .vddi = RTK_DISPLAY_PIN_NONE,
    .avdd = P2_3,
    .bl = RTK_DISPLAY_PIN_NONE,
    .rs = RTK_DISPLAY_PIN_NONE,
    .dbic_speed = 2,
};

const T_RTK_SDI_BUS board_sdi_st7801n_bus =
{
    .clock = RTK_DISPLAY_CLK_200M,
    .pin_group = 1,
    .cs = P9_2,
    .clk = P9_4,
    .d0 = P9_3,
    .d1 = P9_1,
    .d2 = P9_0,
    .d3 = P9_5,
    .te = P2_2,
    .rst = P4_0,
    .vddi = RTK_DISPLAY_PIN_NONE,
    .avdd = RTK_DISPLAY_PIN_NONE,
    .bl = P2_5,
    .rs = RTK_DISPLAY_PIN_NONE,
    .dbic_speed = 2,
};

/* Auto-detect 360x360 QSPI panels (ST77916 / GC9B71).  The original board
 * uses P5_1 as power enable, P5_2 reset, P2_4 RS and P5_0 backlight. */
const T_RTK_SDI_BUS board_sdi_auto_360_bus =
{
    .clock = RTK_DISPLAY_CLK_200M,
    .pin_group = 1,
    .cs = P9_2,
    .clk = P9_4,
    .d0 = P9_3,
    .d1 = P9_1,
    .d2 = P9_0,
    .d3 = P9_5,
    .te = RTK_DISPLAY_PIN_NONE,
    .rst = P5_2,
    .vddi = RTK_DISPLAY_PIN_NONE,
    .avdd = P5_1,
    .bl = P5_0,
    .rs = P2_4,
    .dbic_speed = 2,
};

const T_RTK_RGB_BUS board_rgb_bus =
{
    .clock = RTK_DISPLAY_CLK_200M,
    .pin_group = 0,
    .data =
    {
        P2_6, P2_7, P4_0, P4_1, P4_2, P4_3, P4_4, P4_5,
        P4_6, P4_7, P8_0, P8_1, P8_2, P8_3, P8_4, P8_5,
        P8_6, P8_7, P9_1, P9_2, P9_3, P9_4, P9_5, P9_6,
    },
    .pclk = P2_5,
    .hsync = P2_4,
    .vsync = P2_2,
    .de = P2_3,
    .rst = P7_3,
    .bl = RTK_DISPLAY_PIN_NONE,
    .vddi = RTK_DISPLAY_PIN_NONE,
    .avdd = RTK_DISPLAY_PIN_NONE,
};

int board_display_new(rtk_display_t **out)
{
#if defined(CONFIG_REALTEK_LCD_EK9716_800_480_RGB)
    rtk_rgb_t *rgb = NULL;

    if (rtk_rgb_new_panel_ek9716(&board_rgb_bus, &rgb) != 0)
    {
        return -1;
    }
    return rtk_display_new_rgb(rgb, out);
#elif defined(CONFIG_REALTEK_LCD_JD9165BA_1024_600_RGB)
    rtk_rgb_t *rgb = NULL;

    if (rtk_rgb_new_panel_jd9165ba(&board_rgb_bus, &rgb) != 0)
    {
        return -1;
    }
    return rtk_display_new_rgb(rgb, out);
#else
    rtk_sdi_t *sdi = NULL;

#if defined(CONFIG_REALTEK_LCD_ST7801N_466_466_QSPI)
    if (rtk_sdi_new_panel_st7801n(&board_sdi_st7801n_bus, &sdi) != 0)
    {
        return -1;
    }
#elif defined(CONFIG_REALTEK_LCD_ST77916_360_360_QSPI)
    if (rtk_sdi_new_panel_st77916_360_360(&board_sdi_auto_360_bus, &sdi) != 0)
    {
        return -1;
    }
#elif defined(CONFIG_REALTEK_LCD_GC9B71_360_360_QSPI)
    if (rtk_sdi_new_panel_gc9b71_360_360(&board_sdi_auto_360_bus, &sdi) != 0)
    {
        return -1;
    }
#elif defined(CONFIG_REALTEK_LCD_AUTO_360_360_QSPI)
    if (rtk_sdi_new_panel_auto_360_360(&board_sdi_auto_360_bus, &sdi) != 0)
    {
        return -1;
    }
#else
    if (rtk_sdi_new_panel_sh8601(&board_sdi_sh8601_bus, &sdi) != 0)
    {
        return -1;
    }
#endif
    return rtk_display_new_sdi(sdi, out);
#endif
}
