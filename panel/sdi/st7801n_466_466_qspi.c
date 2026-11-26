/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Panel data for ST7801N 466x466 QSPI.
 *
 * This panel is the 200 MHz / Msize16 reference for the 8773G QSPI bus
 * abstraction.  It also exercises the optional panel ops (brightness,
 * display on/off, partial mode) without adding branches to the controller.
 */

#include "rtk_sdi.h"

static const T_RTK_SDI_CMD st7801n_init_seq[] =
{
    {0x11, 100, 0, {0}},
    {0x53, 0,   1, {0x20}},
    {0x51, 0,   1, {0xF0}},
    {0x3A, 0,   1, {0x55}},             /* RGB565 on wire */
    {0x35, 0,   1, {0x00}},
    {0x29, 20,  0, {0}},
};

static void st7801n_set_brightness(uint8_t brightness)
{
    rtk_sdi_bus_write_cmd(0x51, &brightness, 1);
}

static void st7801n_set_display(bool on)
{
    rtk_sdi_bus_write_cmd(on ? 0x29 : 0x28, NULL, 0);
}

static void st7801n_set_partial(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t rows[4];
    uint8_t cols[4];
    uint16_t sr = y & 0xFFFE;
    uint16_t er = (y + h - 1) & 0xFFFE;
    uint16_t sc = x & 0xFFFE;
    uint16_t ec = (x + w - 1) & 0xFFFE;

    rows[0] = sr >> 8;
    rows[1] = sr & 0xFF;
    rows[2] = er >> 8;
    rows[3] = er & 0xFF;
    rtk_sdi_bus_write_cmd(0x30, rows, sizeof(rows));

    cols[0] = sc >> 8;
    cols[1] = sc & 0xFF;
    cols[2] = ec >> 8;
    cols[3] = ec & 0xFF;
    rtk_sdi_bus_write_cmd(0x31, cols, sizeof(cols));

    rtk_sdi_bus_write_cmd(0x12, NULL, 0);
}

static void st7801n_exit_partial(void)
{
    rtk_sdi_bus_write_cmd(0x13, NULL, 0);
}

static const T_RTK_SDI_OPS st7801n_ops =
{
    .set_brightness = st7801n_set_brightness,
    .set_display = st7801n_set_display,
    .set_partial = st7801n_set_partial,
    .exit_partial = st7801n_exit_partial,
};

static const T_RTK_SDI_PANEL st7801n_466_466_qspi =
{
    .name = "st7801n-466x466-qspi",
    .width = 466,
    .height = 466,
    .input_bits = 16,
    .output_bits = 16,
    .iface = RTK_SDI_IF_QSPI,
    .te = false,
    .te_falling = true,
    .init_seq = st7801n_init_seq,
    .init_count = (uint16_t)(sizeof(st7801n_init_seq) / sizeof(st7801n_init_seq[0])),
    .ops = &st7801n_ops,
    .reset_pre_ms = 100,
    .reset_low_ms = 50,
    .reset_post_ms = 130,
};

int rtk_sdi_new_panel_st7801n(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out)
{
    return rtk_sdi_new_panel(&st7801n_466_466_qspi, bus, out);
}
