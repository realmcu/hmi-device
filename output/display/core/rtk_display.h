/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTK_DISPLAY_H
#define RTK_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include "rtk_sdi.h"
#include "rtk_rgb.h"
#include "rtk_display_irq.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    RTK_DISPLAY_TYPE_SDI = 0,
    RTK_DISPLAY_TYPE_RGB,
} rtk_display_type_t;

typedef struct rtk_display rtk_display_t;

int rtk_display_new_sdi(rtk_sdi_t *sdi, rtk_display_t **out);
int rtk_display_new_rgb(rtk_rgb_t *rgb, rtk_display_t **out);
rtk_display_t *rtk_display_get_default(void);
int rtk_display_init(rtk_display_t *disp);
int rtk_display_deinit(rtk_display_t *disp);
void rtk_display_delete(rtk_display_t *disp);

rtk_display_type_t rtk_display_get_type(const rtk_display_t *disp);
uint32_t rtk_display_get_width(const rtk_display_t *disp);
uint32_t rtk_display_get_height(const rtk_display_t *disp);
uint32_t rtk_display_get_pixel_bits(const rtk_display_t *disp);

/* Commit a complete framebuffer.  len_pixels is the number of pixels
 * (width * height), not bytes.  QSPI programs a full-screen window; RGB swaps
 * the infinite DMA buffer. */
int rtk_display_commit(rtk_display_t *disp, const uint8_t *buf,
                       uint32_t len_pixels);

/* One application-facing submit:
 *   SDI/DBIB: wait previous, arm, start, return (current transfer async).
 *   RGB:      arm, switch address, wait switch done, return (safe to draw next). */
int rtk_display_submit(rtk_display_t *disp, const uint8_t *buf,
                       uint32_t len_pixels, uint32_t timeout_ms);

/* Optional explicit wait after submit.  No-op if submit already completed. */
int rtk_display_wait_complete(rtk_display_t *disp, uint32_t timeout_ms);

/* Synchronous partial-region write for SDI.  RGB panels require a complete
 * framebuffer and return -1. */
int rtk_display_write_region(rtk_display_t *disp,
                             uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h,
                             const uint8_t *buf, uint32_t len_pixels);

bool rtk_display_power_on(rtk_display_t *disp);
bool rtk_display_power_off(rtk_display_t *disp);

#ifdef __cplusplus
}
#endif

#endif /* RTK_DISPLAY_H */
