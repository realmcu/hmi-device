/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "rtk_sdi.h"
#include "rtk_display_soc.h"

#include "rtl_lcdc_dbic.h"
#include "rtl_lcdc_dma.h"



#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

#define RTK_SDI_QSPI_OPCODE_WRITE         (0x02)
#define RTK_SDI_QSPI_CMD_READ_ID          (0x04)
#define RTK_SDI_QSPI_CMD_ID1              (0xDA)
#define RTK_SDI_QSPI_CMD_ID2              (0xDB)
#define RTK_SDI_QSPI_CMD_ID3              (0xDC)

struct rtk_sdi
{
    const T_RTK_SDI_PANEL *panel;
    const T_RTK_SDI_BUS *bus;
    T_LCDC_TE_TYPE te_type;
    bool initialized;
    bool used;
};

static struct rtk_sdi lcd_instance;
static rtk_sdi_t *default_lcd;

static bool lcd_valid(const rtk_sdi_t *lcd)
{
    return lcd == &lcd_instance && lcd->used &&
           lcd->panel != NULL && lcd->bus != NULL;
}

static bool lcd_ready(const rtk_sdi_t *lcd)
{
    return lcd_valid(lcd) && lcd->initialized;
}

int rtk_sdi_new_panel(const T_RTK_SDI_PANEL *panel, const T_RTK_SDI_BUS *bus,
                      rtk_sdi_t **out)
{
    if (panel == NULL || bus == NULL || out == NULL || lcd_instance.used)
    {
        return -1;
    }

    lcd_instance.panel = panel;
    lcd_instance.bus = bus;
    lcd_instance.te_type = LCDC_TE_TYPE_HW_TE;
    lcd_instance.initialized = false;
    lcd_instance.used = true;
    *out = &lcd_instance;
    if (default_lcd == NULL)
    {
        default_lcd = *out;
    }
    return 0;
}

rtk_sdi_t *rtk_sdi_get_default(void)
{
    return default_lcd;
}

static void qspi_pad_config(const T_RTK_SDI_BUS *bus)
{
    rtk_display_soc_pad_bus(bus->cs, false, true);
    rtk_display_soc_pad_bus(bus->clk, false, true);
    rtk_display_soc_pad_bus(bus->d0, false, true);
    rtk_display_soc_pad_bus(bus->d1, false, true);
    rtk_display_soc_pad_bus(bus->d2, false, true);
    rtk_display_soc_pad_bus(bus->d3, false, true);
    rtk_display_soc_pad_bus(bus->te, false, false);

    if (bus->rs != RTK_DISPLAY_PIN_NONE)
    {
        rtk_display_soc_pad_output(bus->rs, true);
    }
}

static void pad_output(uint32_t pin, bool high)
{
    if (pin == RTK_DISPLAY_PIN_NONE)
    {
        return;
    }

    rtk_display_soc_pad_output(pin, high);
}

static void panel_power_rails_on(const rtk_sdi_t *lcd)
{
    pad_output(lcd->bus->vddi, true);
    pad_output(lcd->bus->avdd, true);
}

static void panel_power_rails_off(const rtk_sdi_t *lcd)
{
    pad_output(lcd->bus->avdd, false);
    pad_output(lcd->bus->vddi, false);
}

static void panel_backlight_on(const rtk_sdi_t *lcd)
{
    pad_output(lcd->bus->bl, true);
}

static void panel_backlight_off(const rtk_sdi_t *lcd)
{
    pad_output(lcd->bus->bl, false);
}

/* Power sequence: VDDI/AVDD -> RESET -> INIT -> BL.  BL is enabled by
 * rtk_sdi_init() after the init sequence and clear. */
static void panel_reset(const rtk_sdi_t *lcd)
{
    const T_RTK_SDI_PANEL *p = lcd->panel;
    const T_RTK_SDI_BUS *bus = lcd->bus;

    panel_power_rails_on(lcd);

    if (bus->rst == RTK_DISPLAY_PIN_NONE)
    {
        return;
    }

    rtk_display_soc_pad_output(bus->rst, true);
    rtk_display_soc_delay_ms(p->reset_pre_ms);
    rtk_display_soc_pad_output(bus->rst, false);
    rtk_display_soc_delay_ms(p->reset_low_ms);
    rtk_display_soc_pad_output(bus->rst, true);
    rtk_display_soc_delay_ms(p->reset_post_ms);
}

/* QSPI write frame: 0x02, ADDR[23:16], ADDR[15:8], ADDR[7:0], data... */
static void lcdc_dbic_write(uint8_t *buf, uint32_t len)
{
    DBIC->CTRLR0 |= (1UL << 31);
    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));
    DBIC->CTRLR0 &= ~(BIT_TMOD(3));
    DBIC_TX_NDF(len - 4);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);

    for (uint32_t i = 0; i < len; i++)
    {
        DBIC->DR[0].byte = buf[i];
    }

    DBIC_Cmd(ENABLE);
    while (DBIC->SR & BIT0);
}

static void rtk_sdi_write_cmd(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t buf[4 + RTK_SDI_MAX_PARAM] = {0};

    if (len > RTK_SDI_MAX_PARAM)
    {
        return;
    }

    buf[0] = RTK_SDI_QSPI_OPCODE_WRITE;
    buf[1] = 0;
    buf[2] = cmd;
    buf[3] = 0;

    if (data && len)
    {
        memcpy(&buf[4], data, len);
    }

    lcdc_dbic_write(buf, 4 + len);
}

void rtk_sdi_bus_write_cmd(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    rtk_sdi_write_cmd(cmd, data, len);
}

static void lcdc_dbic_receive_buf(uint16_t addr, uint16_t data_len, uint8_t *data,
                                  uint16_t dummy_len, uint8_t so_dnum)
{
    uint32_t rd_num = 0;
    DBIC_SSIENR_TypeDef dbic_ssienr = {.d32 = DBIC->SSIENR};
    dbic_ssienr.b.spic_en = 0;
    DBIC->SSIENR = dbic_ssienr.d32;

    DBIC_CTRLR0_TypeDef dbic_ctrlr0 = {.d32 = DBIC->CTRLR0};
    dbic_ctrlr0.b.cmd_ch = DBIC_CH_SINGLE;
    dbic_ctrlr0.b.data_ch = DBIC_CH_SINGLE;
    dbic_ctrlr0.b.addr_ch = DBIC_CH_SINGLE;
    dbic_ctrlr0.b.tmod = DBIC_TMODE_RX;
    dbic_ctrlr0.b.user_mode = DBIC_USER_MODE;
    DBIC->CTRLR0 = dbic_ctrlr0.d32;

    DBIC_RX_NDF_TypeDef dbic_rx_ndf = {.d32 = DBIC->RX_NDF};
    dbic_rx_ndf.b.rx_ndf = data_len;
    DBIC->RX_NDF = dbic_rx_ndf.d32;
    DBIC_TX_NDF(0);

    DBIC_USER_LENGTH_TypeDef dbic_user_length = {.d32 = DBIC->USER_LENGTH};
    dbic_user_length.b.user_addr_length = 3;
    dbic_user_length.b.user_cmd_length = 1;
    dbic_user_length.b.user_rd_dummy_length = dummy_len > 0x0FFF ? 0x0FFF : dummy_len;
    DBIC->USER_LENGTH = dbic_user_length.d32;

    addr <<= 8;
    DBIC->DR[0].word = 0x03 | __REV(addr);

    DBIC_CTRLR2_TypeDef dbic_ctrlr2 = {.d32 = DBIC->CTRLR2};
    dbic_ctrlr2.b.so_dnum = so_dnum;
    DBIC->CTRLR2 = dbic_ctrlr2.d32;

    dbic_ssienr.b.spic_en = 1;
    DBIC->SSIENR = dbic_ssienr.d32;

    DBIC_SR_TypeDef dbic_sr;
    while (rd_num < data_len)
    {
        dbic_sr.d32 = DBIC->SR;
        if (dbic_sr.b.rfne)
        {
            data[rd_num++] = DBIC->DR[0].byte;
        }
    }

    dbic_ssienr.b.spic_en = 0;
    DBIC->SSIENR = dbic_ssienr.d32;
}

uint32_t rtk_sdi_read_id(rtk_sdi_t *lcd)
{
    if (!lcd_valid(lcd))
    {
        return 0;
    }

    uint8_t id[3] = {0};
    uint8_t data[4] = {0};

    lcdc_dbic_receive_buf(RTK_SDI_QSPI_CMD_READ_ID, sizeof(data), data, 0, 1);
    id[0] = data[1] & 0x7F;
    id[1] = data[2] & 0x7F;
    id[2] = data[3] & 0x7F;
    uint32_t lcd_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
    if (lcd_id != 0U)
    {
        return lcd_id;
    }

    lcdc_dbic_receive_buf(RTK_SDI_QSPI_CMD_READ_ID, sizeof(data), data, 0, 0);
    id[0] = data[1] & 0x7F;
    id[1] = data[2] & 0x7F;
    id[2] = data[3] & 0x7F;
    lcd_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
    if (lcd_id != 0U)
    {
        return lcd_id;
    }

    lcdc_dbic_receive_buf(RTK_SDI_QSPI_CMD_ID1, 1, &id[0], 0, 1);
    lcdc_dbic_receive_buf(RTK_SDI_QSPI_CMD_ID2, 1, &id[1], 0, 1);
    lcdc_dbic_receive_buf(RTK_SDI_QSPI_CMD_ID3, 1, &id[2], 0, 1);
    id[0] &= 0x7F;
    id[1] &= 0x7F;
    id[2] &= 0x7F;
    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

void rtk_sdi_delay_ms(uint32_t ms)
{
    rtk_display_soc_delay_ms(ms);
}

static void rtk_sdi_send_seq(const T_RTK_SDI_CMD *seq, uint16_t count)
{
    if (seq == NULL)
    {
        return;
    }

    for (uint16_t i = 0; i < count; i++)
    {
        if (seq[i].cmd != 0 || seq[i].len != 0)
        {
            rtk_sdi_write_cmd(seq[i].cmd, seq[i].data, seq[i].len);
        }
        if (seq[i].delay_ms != 0)
        {
            rtk_display_soc_delay_ms(seq[i].delay_ms);
        }
    }
}

static void lcdc_dbic_enter_data_output_mode(uint32_t len_byte)
{
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);
    DBIC_TX_NDF(len_byte);

    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));
    DBIC->CTRLR0 |= (BIT_CMD_CH(0) | BIT_ADDR_CH(0) | BIT_DATA_CH(2));

    DBIC->FLUSH_FIFO = 0x01;

    /* Must push cmd and address to handler before SPIC enable. */
    LCDC_SPICCmd(0x32);
    LCDC_SPICAddr(0x002c00);

    DBIC->DMACR = 2;
    DBIC->DMATDLR = 0x20;
    DBIC->ICR = 1;
    LCDC_AXIMUXMode(LCDC_HW_MODE);
}

int rtk_sdi_init(rtk_sdi_t *lcd)
{
    if (!lcd_valid(lcd))
    {
        return -1;
    }

    if (lcd->initialized)
    {
        return 0;
    }

    const T_RTK_SDI_PANEL *p = lcd->panel;
    const T_RTK_SDI_BUS *bus = lcd->bus;

    qspi_pad_config(bus);

    if (rtk_display_soc_clock_config(bus->clock) != 0)
    {
        return -1;
    }

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DBIC;

    switch (p->input_bits)
    {
    case 32:
        lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_ARGB8888;
        break;
    case 24:
        lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
        break;
    default:
        lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
        break;
    }

    switch (p->output_bits)
    {
    case 24:
        lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
        break;
    default:
        lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
        break;
    }

    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS;
    if (p->te)
    {
        lcdc_init.LCDC_TeEn = ENABLE;
        lcdc_init.LCDC_TePolarity = p->te_falling ? LCDC_TE_EDGE_FALLING :
                                    LCDC_TE_EDGE_RISING;
        lcdc_init.LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
    }
    lcdc_init.LCDC_GroupSel = bus->pin_group;
    lcdc_init.LCDC_DmaThreshold = 64;
    lcdc_init.LCDC_PhaseShift = rtk_display_soc_sdi_phase_shift();

    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);

    LCDC_DBICCfgTypeDef dbic_init = {0};
    dbic_init.DBIC_SPEED_SEL = bus->dbic_speed;
    dbic_init.DBIC_TxThr = 0;
    dbic_init.DBIC_RxThr = 0;
    dbic_init.SCPOL = DBIC_SCPOL_LOW;
    dbic_init.SCPH = DBIC_SCPH_1Edge;
    DBIC_Init(&dbic_init);

    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

    panel_reset(lcd);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);

    if (p->ops && p->ops->init)
    {
        if (p->ops->init() != 0)
        {
            return -1;
        }
    }
    else
    {
        rtk_sdi_send_seq(p->init_seq, p->init_count);
    }

    DBIC_IMR_TypeDef dbic_reg_0x2c = {.d32 = DBIC->IMR};
    dbic_reg_0x2c.b.dreim = 1;
    DBIC->IMR = dbic_reg_0x2c.d32;

    if (rtk_sdi_clear_screen(lcd, 0x00FF0000) != 0)
    {
        return -1;
    }

    /* INIT -> BL: keep the panel dark while the init table runs. */
    panel_backlight_on(lcd);
    lcd->initialized = true;
    return 0;
}

int rtk_sdi_deinit(rtk_sdi_t *lcd)
{
    if (!lcd_valid(lcd))
    {
        return -1;
    }

    panel_backlight_off(lcd);

    if (lcd->initialized)
    {
        const T_RTK_SDI_PANEL *p = lcd->panel;
        if (p->ops && p->ops->power_off)
        {
            (void)p->ops->power_off();
        }
        else
        {
            rtk_sdi_send_seq(p->power_off_seq, p->power_off_count);
        }
    }

    panel_power_rails_off(lcd);
    LCDC_Cmd(DISABLE);
    DBIC_Cmd(DISABLE);
    lcd->initialized = false;
    return 0;
}

void rtk_sdi_delete(rtk_sdi_t *lcd)
{
    if (!lcd_valid(lcd))
    {
        return;
    }

    (void)rtk_sdi_deinit(lcd);
    if (default_lcd == lcd)
    {
        default_lcd = NULL;
    }
    lcd->panel = NULL;
    lcd->bus = NULL;
    lcd->te_type = LCDC_TE_TYPE_NO_TE;
    lcd->initialized = false;
    lcd->used = false;
}

uint32_t rtk_sdi_get_width(const rtk_sdi_t *lcd)
{
    return lcd_valid(lcd) ? lcd->panel->width : 0;
}

uint32_t rtk_sdi_get_height(const rtk_sdi_t *lcd)
{
    return lcd_valid(lcd) ? lcd->panel->height : 0;
}

uint32_t rtk_sdi_get_pixel_bits(const rtk_sdi_t *lcd)
{
    return lcd_valid(lcd) ? lcd->panel->input_bits : 0;
}

bool rtk_sdi_power_off(rtk_sdi_t *lcd)
{
    return rtk_sdi_deinit(lcd) == 0;
}

bool rtk_sdi_power_on(rtk_sdi_t *lcd)
{
    if (!lcd_valid(lcd))
    {
        return false;
    }

    if (lcd->initialized)
    {
        panel_backlight_on(lcd);
        return true;
    }

    return rtk_sdi_init(lcd) == 0;
}

int rtk_sdi_start_transfer(rtk_sdi_t *lcd, uint8_t *buf, uint32_t len)
{
    if (!lcd_ready(lcd))
    {
        return -1;
    }

    uint32_t msize = LCDC_DMA_Msize_64;

    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);

    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize = msize;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize = msize;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr = (uint32_t)buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_ForceBurst(ENABLE);
    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);

    if (lcd->panel->te)
    {
        if (lcd->te_type == LCDC_TE_TYPE_HW_TE)
        {
            LCDC_TeCmd(ENABLE);
            LCDC_TeEnableDMA(DISABLE);
        }
        else
        {
            LCDC_AutoWriteCmd(ENABLE);
        }
    }
    else
    {
        LCDC_AutoWriteCmd(ENABLE);
    }

    return 0;
}

int rtk_sdi_transfer_cleanup(rtk_sdi_t *lcd)
{
    if (!lcd_ready(lcd))
    {
        return -1;
    }

    if (lcd->panel->te && lcd->te_type == LCDC_TE_TYPE_HW_TE)
    {
        LCDC_TeCmd(DISABLE);
    }

    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_Cmd(DISABLE);

    DBIC_FLUSH_FIFO_TypeDef dbic_reg_0x128 = {.d32 = DBIC->FLUSH_FIFO};
    dbic_reg_0x128.b.flush_dr_fifo = 1;
    DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;
    LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);

    return 0;
}

int rtk_sdi_set_window(rtk_sdi_t *lcd, uint16_t xStart, uint16_t yStart,
                       uint16_t w, uint16_t h)
{
    if (!lcd_valid(lcd))
    {
        return -1;
    }

    uint8_t data[4];
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;

    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    rtk_sdi_write_cmd(0x2A, data, 4);

    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    rtk_sdi_write_cmd(0x2B, data, 4);

    uint32_t len_byte = (uint32_t)(xEnd - xStart + 1) * (yEnd - yStart + 1) *
                        lcd->panel->output_bits / 8;
    lcdc_dbic_enter_data_output_mode(len_byte);

    return 0;
}

int rtk_sdi_clear_screen(rtk_sdi_t *lcd, uint32_t ARGB_color)
{
    if (!lcd_valid(lcd))
    {
        return -1;
    }

    const T_RTK_SDI_PANEL *p = lcd->panel;
    uint8_t *RGB_transfer = (uint8_t *)&ARGB_color;
    uint32_t clear_buf[64] = {0};
    uint32_t msize = LCDC_DMA_Msize_16;

    (void)rtk_sdi_set_window(lcd, 0, 0, p->width, p->height);

    if (p->input_bits == 32)
    {
        uint32_t rgba8888 = RGB_transfer[3] << 24 | RGB_transfer[0] << 16 |
                            RGB_transfer[1] << 8 | RGB_transfer[2];
        for (int i = 0; i < 64; i++)
        {
            clear_buf[i] = rgba8888;
        }
    }
    else if (p->input_bits == 24)
    {
        uint8_t *rgb888_buf = (uint8_t *)clear_buf;
        for (int i = 0; i < 64; i++)
        {
            rgb888_buf[i * 3] = RGB_transfer[0];
            rgb888_buf[i * 3 + 1] = RGB_transfer[1];
            rgb888_buf[i * 3 + 2] = RGB_transfer[2];
        }
    }
    else
    {
        uint16_t *rgb565_buf = (uint16_t *)clear_buf;
        uint16_t color = (((RGB_transfer[0] & 0xF8) << 8) |
                          ((RGB_transfer[1] & 0xFC) << 3) |
                          ((RGB_transfer[2] & 0xF8) >> 3));
        for (int i = 0; i < 64 * 2; i++)
        {
            rgb565_buf[i] = color;
        }
    }

    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc = LCDC_DMA_SourceInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize = msize;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize = msize;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr = (uint32_t)clear_buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(p->height * p->width);

    LCDC_Cmd(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);

    while ((LCDC_HANDLER->DMA_FIFO_CTRL & LCDC_DMA_ENABLE) != RESET);

    LCDC_HANDLER_OPERATE_CTR_t handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);

    rtk_display_soc_delay_us(10);
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);

    return 0;
}

int rtk_sdi_enter_dlps(rtk_sdi_t *lcd)
{
    if (!lcd_valid(lcd))
    {
        return -1;
    }

    const T_RTK_SDI_BUS *bus = lcd->bus;
    const uint32_t pins[] =
    {
        bus->cs, bus->clk, bus->d0, bus->d1, bus->d2, bus->d3, bus->te
    };

    for (uint32_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        if (pins[i] == RTK_DISPLAY_PIN_NONE)
        {
            continue;
        }
        rtk_display_soc_pad_sleep(pins[i]);
    }

    return 0;
}

void rtk_sdi_set_TE_type(rtk_sdi_t *lcd, T_LCDC_TE_TYPE state)
{
    if (lcd_valid(lcd))
    {
        lcd->te_type = state;
    }
}

T_LCDC_TE_TYPE rtk_sdi_get_TE_type(const rtk_sdi_t *lcd)
{
    return lcd_valid(lcd) ? lcd->te_type : LCDC_TE_TYPE_NO_TE;
}

void rtk_sdi_set_brightness(rtk_sdi_t *lcd, uint8_t brightness)
{
    if (lcd_valid(lcd) && lcd->panel->ops && lcd->panel->ops->set_brightness)
    {
        lcd->panel->ops->set_brightness(brightness);
    }
}

void rtk_sdi_set_display(rtk_sdi_t *lcd, bool on)
{
    if (lcd_valid(lcd) && lcd->panel->ops && lcd->panel->ops->set_display)
    {
        lcd->panel->ops->set_display(on);
    }
}

void rtk_sdi_set_partial_mode(rtk_sdi_t *lcd, uint16_t x, uint16_t y,
                              uint16_t w, uint16_t h)
{
    if (lcd_valid(lcd) && lcd->panel->ops && lcd->panel->ops->set_partial)
    {
        lcd->panel->ops->set_partial(x, y, w, h);
    }
}

void rtk_sdi_exit_partial_mode(rtk_sdi_t *lcd)
{
    if (lcd_valid(lcd) && lcd->panel->ops && lcd->panel->ops->exit_partial)
    {
        lcd->panel->ops->exit_partial();
    }
}
