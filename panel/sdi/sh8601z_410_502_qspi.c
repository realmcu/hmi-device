/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Panel data for SH8601Z 410x502 QSPI.
 *
 * This file intentionally contains no LCDC/DBIC/DMA or board pin code.  It
 * describes the panel (geometry, pixel formats, reset timing, command
 * sequences).  Pins/clock/DMA live in the board T_RTK_SDI_BUS.  The controller
 * is device/core/rtk_sdi_8773g.c.
 */

#include "rtk_sdi.h"

static const T_RTK_SDI_CMD sh8601z_init_seq[] =
{
    {0x11, 5,  0, {0}},
    {0xC0, 0,  2, {0x5A, 0x5A}},
    {0xB0, 0,  1, {0x33}},
    {0xB1, 0,  1, {0x00}},
    {0x2A, 0,  4, {0x00, 0x00, 0x01, 0x99}},
    {0x2B, 0,  4, {0x00, 0x00, 0x01, 0xF5}},
    {0x44, 0,  2, {0x00, 0x09}},
    {0x3A, 0,  1, {0x55}},             /* RGB565 on wire */
    {0x35, 0,  1, {0x00}},
    {0x53, 25, 1, {0x20}},
    {0x29, 0,  0, {0}},
};

static const T_RTK_SDI_CMD sh8601z_power_on_seq[] =
{
    {0x29, 0, 0, {0}},
    {0x11, 5, 0, {0}},
};

static const T_RTK_SDI_CMD sh8601z_power_off_seq[] =
{
    {0x28, 25, 0, {0}},
    {0x10, 50, 0, {0}},
};

static const T_RTK_SDI_PANEL sh8601z_410_502_qspi =
{
    .name = "sh8601z-410x502-qspi",
    .width = 410,
    .height = 502,
    .input_bits = 16,
    .output_bits = 16,
    .iface = RTK_SDI_IF_QSPI,
    .te = true,
    .te_falling = true,
    .init_seq = sh8601z_init_seq,
    .init_count = (uint16_t)(sizeof(sh8601z_init_seq) / sizeof(sh8601z_init_seq[0])),
    .power_on_seq = sh8601z_power_on_seq,
    .power_on_count = (uint16_t)(sizeof(sh8601z_power_on_seq) /
                                 sizeof(sh8601z_power_on_seq[0])),
    .power_off_seq = sh8601z_power_off_seq,
    .power_off_count = (uint16_t)(sizeof(sh8601z_power_off_seq) /
                                  sizeof(sh8601z_power_off_seq[0])),
    .ops = NULL,
    .reset_pre_ms = 100,
    .reset_low_ms = 50,
    .reset_post_ms = 50,
};

int rtk_sdi_new_panel_sh8601(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out)
{
    return rtk_sdi_new_panel(&sh8601z_410_502_qspi, bus, out);
}
