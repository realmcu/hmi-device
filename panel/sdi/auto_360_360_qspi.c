/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Runtime auto-detect for the two 360x360 QSPI panels:
 *   ST77916 (ID 0x003F7F4F) and GC9B71 (ID 0x004D387F).
 */

#include "rtk_sdi.h"

#define LCD_PANEL_ID_ST77916 0x003F7F4Fu
#define LCD_PANEL_ID_GC9B71  0x004D387Fu

extern const T_RTK_SDI_CMD rtk_sdi_st77916_360_init_seq[];
extern const uint16_t rtk_sdi_st77916_360_init_seq_count;
extern const T_RTK_SDI_CMD rtk_sdi_gc9b71_360_init_seq[];
extern const uint16_t rtk_sdi_gc9b71_360_init_seq_count;

static void send_seq(const T_RTK_SDI_CMD *seq, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++)
    {
        if (seq[i].cmd != 0 || seq[i].len != 0)
        {
            rtk_sdi_bus_write_cmd(seq[i].cmd, seq[i].data, seq[i].len);
        }
        if (seq[i].delay_ms != 0)
        {
            rtk_sdi_delay_ms(seq[i].delay_ms);
        }
    }
}

static int auto_360_360_init(void)
{
    rtk_sdi_t *lcd = rtk_sdi_get_default();
    uint32_t id = rtk_sdi_read_id(lcd);

    if (id == LCD_PANEL_ID_ST77916)
    {
        send_seq(rtk_sdi_st77916_360_init_seq, rtk_sdi_st77916_360_init_seq_count);
        return 0;
    }
    if (id == LCD_PANEL_ID_GC9B71)
    {
        send_seq(rtk_sdi_gc9b71_360_init_seq, rtk_sdi_gc9b71_360_init_seq_count);
        return 0;
    }
    return -1;
}

static const T_RTK_SDI_OPS auto_360_360_ops =
{
    .init = auto_360_360_init,
};

static const T_RTK_SDI_PANEL auto_360_360_qspi =
{
    .name = "auto-360x360-qspi",
    .width = 360,
    .height = 360,
    .input_bits = 16,
    .output_bits = 16,
    .iface = RTK_SDI_IF_QSPI,
    .te = false,
    .te_falling = false,
    .init_seq = NULL,
    .init_count = 0,
    .ops = &auto_360_360_ops,
    .reset_pre_ms = 100,
    .reset_low_ms = 50,
    .reset_post_ms = 130,
};

int rtk_sdi_new_panel_auto_360_360(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out)
{
    return rtk_sdi_new_panel(&auto_360_360_qspi, bus, out);
}
