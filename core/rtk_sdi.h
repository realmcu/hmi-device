/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTK_SDI_H
#define RTK_SDI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtk_display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTK_SDI_MAX_PARAM 60

typedef enum
{
    LCDC_TE_TYPE_NO_TE = 0x00,
    LCDC_TE_TYPE_HW_TE = 0x01,
    LCDC_TE_TYPE_SW_TE = 0x02,
} T_LCDC_TE_TYPE;

typedef enum
{
    RTK_SDI_IF_QSPI = 0,
    RTK_SDI_IF_SPI,
    RTK_SDI_IF_RGB,
    RTK_SDI_IF_DBIB,
} T_RTK_SDI_IF;

typedef struct
{
    T_RTK_DISPLAY_CLK clock;
    uint8_t pin_group;
    uint32_t cs;
    uint32_t clk;
    uint32_t d0;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
    uint32_t te;
    uint32_t rst;
    uint32_t vddi;
    uint32_t avdd;
    uint32_t bl;
    uint32_t rs;
    uint8_t dbic_speed;
} T_RTK_SDI_BUS;

typedef struct
{
    uint8_t cmd;
    uint16_t delay_ms;
    uint16_t len;
    uint8_t data[RTK_SDI_MAX_PARAM];
} T_RTK_SDI_CMD;

/* Optional panel operations.  Panels that only need an init table leave
 * these NULL; panels with brightness/partial/display commands fill them. */
typedef struct
{
    int (*init)(void);
    int (*power_on)(void);
    int (*power_off)(void);
    void (*set_brightness)(uint8_t brightness);
    void (*set_display)(bool on);
    void (*set_partial)(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void (*exit_partial)(void);
} T_RTK_SDI_OPS;

typedef struct
{
    const char *name;
    uint16_t width;
    uint16_t height;
    uint8_t input_bits;
    uint8_t output_bits;
    T_RTK_SDI_IF iface;
    bool te;
    bool te_falling;

    const T_RTK_SDI_CMD *init_seq;
    uint16_t init_count;
    const T_RTK_SDI_CMD *power_on_seq;
    uint16_t power_on_count;
    const T_RTK_SDI_CMD *power_off_seq;
    uint16_t power_off_count;

    const T_RTK_SDI_OPS *ops;

    uint16_t reset_pre_ms;
    uint16_t reset_low_ms;
    uint16_t reset_post_ms;
} T_RTK_SDI_PANEL;

typedef struct rtk_sdi rtk_sdi_t;

int rtk_sdi_new_panel(const T_RTK_SDI_PANEL *panel, const T_RTK_SDI_BUS *bus,
                      rtk_sdi_t **out);
int rtk_sdi_new_panel_sh8601(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out);
int rtk_sdi_new_panel_st7801n(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out);
int rtk_sdi_new_panel_st77916_360_360(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out);
int rtk_sdi_new_panel_gc9b71_360_360(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out);
int rtk_sdi_new_panel_auto_360_360(const T_RTK_SDI_BUS *bus, rtk_sdi_t **out);

/* Used by panel ops such as auto-detect. */
rtk_sdi_t *rtk_sdi_get_default(void);

int rtk_sdi_init(rtk_sdi_t *lcd);
int rtk_sdi_deinit(rtk_sdi_t *lcd);
void rtk_sdi_delete(rtk_sdi_t *lcd);
uint32_t rtk_sdi_get_width(const rtk_sdi_t *lcd);
uint32_t rtk_sdi_get_height(const rtk_sdi_t *lcd);
uint32_t rtk_sdi_get_pixel_bits(const rtk_sdi_t *lcd);
uint32_t rtk_sdi_read_id(rtk_sdi_t *lcd);
bool rtk_sdi_power_off(rtk_sdi_t *lcd);
bool rtk_sdi_power_on(rtk_sdi_t *lcd);
int rtk_sdi_set_window(rtk_sdi_t *lcd, uint16_t xStart, uint16_t yStart,
                       uint16_t w, uint16_t h);
int rtk_sdi_clear_screen(rtk_sdi_t *lcd, uint32_t ARGB_color);
int rtk_sdi_start_transfer(rtk_sdi_t *lcd, uint8_t *buf, uint32_t len);
int rtk_sdi_transfer_cleanup(rtk_sdi_t *lcd);
int rtk_sdi_enter_dlps(rtk_sdi_t *lcd);
void rtk_sdi_set_TE_type(rtk_sdi_t *lcd, T_LCDC_TE_TYPE state);
T_LCDC_TE_TYPE rtk_sdi_get_TE_type(const rtk_sdi_t *lcd);
void rtk_sdi_set_brightness(rtk_sdi_t *lcd, uint8_t brightness);
void rtk_sdi_set_display(rtk_sdi_t *lcd, bool on);
void rtk_sdi_set_partial_mode(rtk_sdi_t *lcd, uint16_t x, uint16_t y,
                              uint16_t w, uint16_t h);
void rtk_sdi_exit_partial_mode(rtk_sdi_t *lcd);

/* Controller helpers used by optional panel ops. */
void rtk_sdi_bus_write_cmd(uint8_t cmd, const uint8_t *data, uint16_t len);
void rtk_sdi_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* RTK_SDI_H */
