/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTK_RGB_H
#define RTK_RGB_H

#include <stdbool.h>
#include <stdint.h>

#include "rtk_display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtk_rgb rtk_rgb_t;

typedef struct
{
    T_RTK_DISPLAY_CLK clock;
    uint8_t pin_group;
    uint32_t data[24];
    uint32_t pclk;
    uint32_t hsync;
    uint32_t vsync;
    uint32_t de;
    uint32_t rst;
    uint32_t bl;
    uint32_t vddi;
    uint32_t avdd;
} T_RTK_RGB_BUS;

typedef struct
{
    const char *name;
    uint16_t width;
    uint16_t height;
    uint8_t input_bits;
    uint8_t output_bits;
    uint8_t clock_div;

    uint16_t hsa;
    uint16_t hbp;
    uint16_t hfp;
    uint16_t vsa;
    uint16_t vbp;
    uint16_t vfp;

    bool hsync_polarity;
    bool vsync_polarity;
    bool de_polarity;

    uint16_t reset_pre_ms;
    uint16_t reset_low_ms;
    uint16_t reset_post_ms;
} T_RTK_RGB_PANEL;

int rtk_rgb_new_panel(const T_RTK_RGB_PANEL *panel, const T_RTK_RGB_BUS *bus,
                      rtk_rgb_t **out);
int rtk_rgb_new_panel_ek9716(const T_RTK_RGB_BUS *bus, rtk_rgb_t **out);
int rtk_rgb_new_panel_jd9165ba(const T_RTK_RGB_BUS *bus, rtk_rgb_t **out);

int rtk_rgb_init(rtk_rgb_t *rgb);
int rtk_rgb_deinit(rtk_rgb_t *rgb);
void rtk_rgb_delete(rtk_rgb_t *rgb);

uint32_t rtk_rgb_get_width(const rtk_rgb_t *rgb);
uint32_t rtk_rgb_get_height(const rtk_rgb_t *rgb);
uint32_t rtk_rgb_get_pixel_bits(const rtk_rgb_t *rgb);
bool rtk_rgb_power_on(rtk_rgb_t *rgb);
bool rtk_rgb_power_off(rtk_rgb_t *rgb);

/* The first call starts the infinite-buffer RGB stream; later calls swap the
 * active framebuffer.  len_pixels must be at least width * height. */
int rtk_rgb_update_framebuffer(rtk_rgb_t *rgb, uint8_t *buf, uint32_t len_pixels);

/* Confirm that RGB DMA has moved into the newly selected buffer. */
void rtk_rgb_confirm_switch(rtk_rgb_t *rgb, const uint8_t *buf, uint32_t len_bytes);

#ifdef __cplusplus
}
#endif

#endif /* RTK_RGB_H */
