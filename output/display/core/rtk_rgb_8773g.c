/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "rtk_rgb.h"

#include "rtl_lcdc_edpi.h"
#include "rtl_lcdc_dma.h"

#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "platform_utils.h"

#define RTK_RGB_DMA_CHANNEL_NUM   0
#define RTK_RGB_DMA_CHANNEL_INDEX LCDC_DMA_Channel0

struct rtk_rgb
{
    const T_RTK_RGB_PANEL *panel;
    const T_RTK_RGB_BUS *bus;
    bool initialized;
    bool stream_started;
    bool used;
};

static struct rtk_rgb rgb_instance;

static bool rgb_valid(const rtk_rgb_t *rgb)
{
    return rgb == &rgb_instance && rgb->used &&
           rgb->panel != NULL && rgb->bus != NULL;
}

int rtk_rgb_new_panel(const T_RTK_RGB_PANEL *panel, const T_RTK_RGB_BUS *bus,
                      rtk_rgb_t **out)
{
    if (panel == NULL || bus == NULL || out == NULL || rgb_instance.used)
    {
        return -1;
    }

    rgb_instance.panel = panel;
    rgb_instance.bus = bus;
    rgb_instance.initialized = false;
    rgb_instance.stream_started = false;
    rgb_instance.used = true;
    *out = &rgb_instance;
    return 0;
}

void rtk_rgb_delete(rtk_rgb_t *rgb)
{
    if (!rgb_valid(rgb))
    {
        return;
    }
    (void)rtk_rgb_deinit(rgb);
    rgb->panel = NULL;
    rgb->bus = NULL;
    rgb->used = false;
}

static void rgb_pad_config(const T_RTK_RGB_BUS *bus)
{
    for (uint32_t i = 0; i < 24; i++)
    {
        if (bus->data[i] == RTK_DISPLAY_PIN_NONE)
        {
            continue;
        }
        Pad_Config(bus->data[i], PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
                   PAD_OUT_DISABLE, PAD_OUT_HIGH);
        Pad_HighSpeedFuncSel(bus->data[i], HS_Func0);
        Pad_HighSpeedMuxSel(bus->data[i], FROM_CORE_DOMAIN);
    }

    const uint32_t ctrl[] = { bus->pclk, bus->hsync, bus->vsync, bus->de };
    for (uint32_t i = 0; i < sizeof(ctrl) / sizeof(ctrl[0]); i++)
    {
        if (ctrl[i] == RTK_DISPLAY_PIN_NONE)
        {
            continue;
        }
        Pad_Config(ctrl[i], PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
                   PAD_OUT_DISABLE, PAD_OUT_HIGH);
        Pad_HighSpeedFuncSel(ctrl[i], HS_Func0);
        Pad_HighSpeedMuxSel(ctrl[i], FROM_CORE_DOMAIN);
    }

    if (bus->rst != RTK_DISPLAY_PIN_NONE)
    {
        Pad_Config(bus->rst, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

static void rgb_reset(const rtk_rgb_t *rgb)
{
    const T_RTK_RGB_PANEL *p = rgb->panel;
    const T_RTK_RGB_BUS *bus = rgb->bus;

    if (bus->vddi != RTK_DISPLAY_PIN_NONE)
    {
        Pad_Config(bus->vddi, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    if (bus->avdd != RTK_DISPLAY_PIN_NONE)
    {
        Pad_Config(bus->avdd, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    if (bus->rst == RTK_DISPLAY_PIN_NONE)
    {
        return;
    }

    Pad_Config(bus->rst, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
               PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(p->reset_pre_ms);
    Pad_Config(bus->rst, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
               PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(p->reset_low_ms);
    Pad_Config(bus->rst, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
               PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(p->reset_post_ms);
}

int rtk_rgb_init(rtk_rgb_t *rgb)
{
    if (!rgb_valid(rgb))
    {
        return -1;
    }
    if (rgb->initialized)
    {
        return 0;
    }

    const T_RTK_RGB_PANEL *p = rgb->panel;
    const T_RTK_RGB_BUS *bus = rgb->bus;

    rgb_pad_config(bus);
    LCDC_Clock_Sel((bus->clock == RTK_DISPLAY_CLK_200M) ? LCDC_BUS_CLK_200M :
                   LCDC_BUS_CLK_280M);

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DPI;
    lcdc_init.LCDC_PixelInputFormat = (p->input_bits == 16) ?
                                      LCDC_INPUT_RGB565 : LCDC_INPUT_RGB888;
    lcdc_init.LCDC_PixelOutputFormat = (p->output_bits == 24) ?
                                       LCDC_OUTPUT_RGB888 : LCDC_OUTPUT_RGB565;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS;
    lcdc_init.LCDC_GroupSel = bus->pin_group;
    lcdc_init.LCDC_DmaThreshold = 64;
    lcdc_init.LCDC_InfiniteModeEn = 1;
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);

    LCDC_eDPICfgTypeDef edpi = {0};
    edpi.eDPI_ClockDiv = p->clock_div;
    edpi.eDPI_HoriSyncWidth = p->hsa;
    edpi.eDPI_VeriSyncHeight = p->vsa;
    edpi.eDPI_AccumulatedHBP = p->hsa + p->hbp;
    edpi.eDPI_AccumulatedVBP = p->vsa + p->vbp;
    edpi.eDPI_AccumulatedActiveW = p->hsa + p->hbp + p->width;
    edpi.eDPI_AccumulatedActiveH = p->vsa + p->vbp + p->height;
    edpi.eDPI_TotalWidth = p->hsa + p->hbp + p->width + p->hfp;
    edpi.eDPI_TotalHeight = p->vsa + p->vbp + p->height + p->vfp;
    edpi.eDPI_HoriSyncPolarity = p->hsync_polarity;
    edpi.eDPI_VeriSyncPolarity = p->vsync_polarity;
    edpi.eDPI_DataEnPolarity = p->de_polarity;
    edpi.eDPI_LineIntMask = 1;
    edpi.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB888;
    edpi.eDPI_OperateMode = 0;
    edpi.eDPI_LineBufferPixelThreshold = edpi.eDPI_TotalWidth / 2;
    EDPI_Init(&edpi);

    rgb_reset(rgb);
    if (bus->bl != RTK_DISPLAY_PIN_NONE)
    {
        Pad_Config(bus->bl, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }

    rgb->initialized = true;
    return 0;
}

int rtk_rgb_deinit(rtk_rgb_t *rgb)
{
    if (!rgb_valid(rgb))
    {
        return -1;
    }

    LCDC_Cmd(DISABLE);
    LCDC_DmaCmd(DISABLE);
    rgb->initialized = false;
    rgb->stream_started = false;
    return 0;
}

uint32_t rtk_rgb_get_width(const rtk_rgb_t *rgb)
{
    return rgb_valid(rgb) ? rgb->panel->width : 0;
}

uint32_t rtk_rgb_get_height(const rtk_rgb_t *rgb)
{
    return rgb_valid(rgb) ? rgb->panel->height : 0;
}

uint32_t rtk_rgb_get_pixel_bits(const rtk_rgb_t *rgb)
{
    return rgb_valid(rgb) ? rgb->panel->input_bits : 0;
}

bool rtk_rgb_power_off(rtk_rgb_t *rgb)
{
    return rtk_rgb_deinit(rgb) == 0;
}

bool rtk_rgb_power_on(rtk_rgb_t *rgb)
{
    return rtk_rgb_init(rgb) == 0;
}

static void rgb_dma_start(rtk_rgb_t *rgb, uint8_t *buf)
{
    const T_RTK_RGB_PANEL *p = rgb->panel;
    uint32_t stride = p->width * p->input_bits / 8;

    LCDC_DMA_InitTypeDef dma = {0};
    LCDC_DMA_StructInit(&dma);
    dma.LCDC_DMA_ChannelNum = RTK_RGB_DMA_CHANNEL_NUM;
    dma.LCDC_DMA_DIR = LCDC_DMA_DIR_PeripheralToMemory;
    dma.LCDC_DMA_SourceInc = LCDC_DMA_SourceInc_Inc;
    dma.LCDC_DMA_DestinationInc = LCDC_DMA_DestinationInc_Fix;
    dma.LCDC_DMA_SourceDataSize = LCDC_DMA_DataSize_Word;
    dma.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    dma.LCDC_DMA_SourceMsize = LCDC_DMA_Msize_64;
    dma.LCDC_DMA_DestinationMsize = LCDC_DMA_Msize_64;
    dma.LCDC_DMA_Multi_Block_Mode = LLI_TRANSFER;
    dma.LCDC_DMA_Multi_Block_En = 1;
    dma.LCDC_DMA_Multi_Block_Struct = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
    LCDC_DMA_Init(RTK_RGB_DMA_CHANNEL_INDEX, &dma);

    LCDC_SET_GROUP1_BLOCKSIZE(stride);
    LCDC_SET_GROUP2_BLOCKSIZE(stride);

    LCDC_DMALLI_InitTypeDef lli = {0};
    lli.g1_source_addr = (uint32_t)buf;
    lli.g2_source_addr = (uint32_t)(buf + stride);
    lli.g1_sar_offset = stride * 2;
    lli.g2_sar_offset = stride * 2;

    LCDC_DMA_Infinite_Buf_Update(buf, buf + stride);
    LCDC_DMA_LinkList_Init(&lli, &dma);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_SetTxPixelLen(p->width * p->height);
    LCDC_ForceBurst(ENABLE);
    LCDC_Cmd(ENABLE);
    LCDC_DMA_SetSourceAddress(RTK_RGB_DMA_CHANNEL_INDEX, (uint32_t)buf);
    LCDC_DMA_MultiBlockCmd(ENABLE);
    LCDC_DMAChannelCmd(RTK_RGB_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);
}

int rtk_rgb_update_framebuffer(rtk_rgb_t *rgb, uint8_t *buf, uint32_t len_pixels)
{
    if (!rgb_valid(rgb) || !rgb->initialized || buf == NULL)
    {
        return -1;
    }

    const T_RTK_RGB_PANEL *p = rgb->panel;
    if (len_pixels < (uint32_t)p->width * p->height)
    {
        return -1;
    }

    uint32_t stride = p->width * p->input_bits / 8;
    if (!rgb->stream_started)
    {
        rgb_dma_start(rgb, buf);
        rgb->stream_started = true;
    }
    else
    {
        LCDC_DMA_Infinite_Buf_Update(buf, buf + stride);
    }
    return 0;
}

void rtk_rgb_confirm_switch(rtk_rgb_t *rgb, const uint8_t *buf, uint32_t len)
{
    if (!rgb_valid(rgb) || !rgb->stream_started || buf == NULL)
    {
        return;
    }

    uintptr_t start = (uintptr_t)buf;
    uintptr_t end = start + len;
    uint32_t sar;

    do
    {
        sar = LCDC_DMA_Channel0->LCDC_DMA_CURR_SARx;
    }
    while (sar < start || sar >= end);
}
