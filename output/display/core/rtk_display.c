/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtk_display.h"

struct rtk_display
{
    rtk_display_type_t type;
    union
    {
        rtk_sdi_t *sdi;
        rtk_rgb_t *rgb;
    } u;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_bits;
    bool used;
    bool pending;
    const uint8_t *switch_buf;
    uint32_t switch_bytes;
};

static struct rtk_display display_slot;

rtk_display_t *rtk_display_get_default(void)
{
    return display_slot.used ? &display_slot : NULL;
}

static bool display_valid(const rtk_display_t *disp)
{
    return disp == &display_slot && disp->used;
}

int rtk_display_new_sdi(rtk_sdi_t *sdi, rtk_display_t **out)
{
    if (sdi == NULL || out == NULL || display_slot.used)
    {
        return -1;
    }

    display_slot.type = RTK_DISPLAY_TYPE_SDI;
    display_slot.u.sdi = sdi;
    display_slot.width = rtk_sdi_get_width(sdi);
    display_slot.height = rtk_sdi_get_height(sdi);
    display_slot.pixel_bits = rtk_sdi_get_pixel_bits(sdi);
    display_slot.used = true;
    display_slot.pending = false;
    *out = &display_slot;
    return 0;
}

int rtk_display_new_rgb(rtk_rgb_t *rgb, rtk_display_t **out)
{
    if (rgb == NULL || out == NULL || display_slot.used)
    {
        return -1;
    }

    display_slot.type = RTK_DISPLAY_TYPE_RGB;
    display_slot.u.rgb = rgb;
    display_slot.width = rtk_rgb_get_width(rgb);
    display_slot.height = rtk_rgb_get_height(rgb);
    display_slot.pixel_bits = rtk_rgb_get_pixel_bits(rgb);
    display_slot.used = true;
    display_slot.pending = false;
    *out = &display_slot;
    return 0;
}

int rtk_display_init(rtk_display_t *disp)
{
    if (!display_valid(disp))
    {
        return -1;
    }

    int ret;

    if (disp->type == RTK_DISPLAY_TYPE_SDI)
    {
        ret = rtk_sdi_init(disp->u.sdi);
    }
    else
    {
        ret = rtk_rgb_init(disp->u.rgb);
    }
    if (ret != 0)
    {
        return ret;
    }

    return rtk_display_irq_init();
}

int rtk_display_deinit(rtk_display_t *disp)
{
    if (!display_valid(disp))
    {
        return -1;
    }

    if (disp->pending &&
        rtk_display_wait_complete(disp, RTK_DISPLAY_WAIT_FOREVER) != 0)
    {
        return -1;
    }

    if (disp->type == RTK_DISPLAY_TYPE_SDI)
    {
        return rtk_sdi_deinit(disp->u.sdi);
    }
    return rtk_rgb_deinit(disp->u.rgb);
}

void rtk_display_delete(rtk_display_t *disp)
{
    if (!display_valid(disp))
    {
        return;
    }

    if (disp->pending)
    {
        (void)rtk_display_wait_complete(disp, RTK_DISPLAY_WAIT_FOREVER);
    }

    if (disp->type == RTK_DISPLAY_TYPE_SDI)
    {
        rtk_sdi_delete(disp->u.sdi);
    }
    else
    {
        rtk_rgb_delete(disp->u.rgb);
    }

    disp->u.sdi = NULL;
    disp->used = false;
}

rtk_display_type_t rtk_display_get_type(const rtk_display_t *disp)
{
    return display_valid(disp) ? disp->type : RTK_DISPLAY_TYPE_SDI;
}

uint32_t rtk_display_get_width(const rtk_display_t *disp)
{
    return display_valid(disp) ? disp->width : 0;
}

uint32_t rtk_display_get_height(const rtk_display_t *disp)
{
    return display_valid(disp) ? disp->height : 0;
}

uint32_t rtk_display_get_pixel_bits(const rtk_display_t *disp)
{
    return display_valid(disp) ? disp->pixel_bits : 0;
}

int rtk_display_wait_complete(rtk_display_t *disp, uint32_t timeout_ms)
{
    if (!display_valid(disp))
    {
        return -1;
    }

    if (!disp->pending)
    {
        return 0;
    }

    if (rtk_display_irq_wait(timeout_ms) != 0)
    {
        return -1;
    }

    if (disp->type == RTK_DISPLAY_TYPE_SDI)
    {
        if (rtk_sdi_transfer_cleanup(disp->u.sdi) != 0)
        {
            return -1;
        }
    }
    else
    {
        rtk_rgb_confirm_switch(disp->u.rgb, disp->switch_buf,
                               disp->switch_bytes);
    }

    disp->pending = false;
    return 0;
}

int rtk_display_submit(rtk_display_t *disp, const uint8_t *buf,
                       uint32_t len_pixels, uint32_t timeout_ms)
{
    if (!display_valid(disp) || buf == NULL)
    {
        return -1;
    }

    if (len_pixels < (uint32_t)disp->width * disp->height)
    {
        return -1;
    }

    if (disp->pending)
    {
        int ret = rtk_display_wait_complete(disp, timeout_ms);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (rtk_display_irq_prepare() != 0)
    {
        return -1;
    }

    if (disp->type == RTK_DISPLAY_TYPE_SDI)
    {
        rtk_sdi_set_TE_type(disp->u.sdi, LCDC_TE_TYPE_NO_TE);
        if (rtk_sdi_set_window(disp->u.sdi, 0, 0,
                               disp->width, disp->height) != 0)
        {
            return -1;
        }
        if (rtk_sdi_start_transfer(disp->u.sdi, (uint8_t *)buf,
                                   len_pixels) != 0)
        {
            return -1;
        }

        disp->pending = true;
        return 0;
    }

    disp->switch_buf = buf;
    disp->switch_bytes = len_pixels * (disp->pixel_bits / 8);
    if (rtk_rgb_update_framebuffer(disp->u.rgb, (uint8_t *)buf,
                                   len_pixels) != 0)
    {
        return -1;
    }

    disp->pending = true;
    return rtk_display_wait_complete(disp, timeout_ms);
}

int rtk_display_write_region(rtk_display_t *disp,
                             uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h,
                             const uint8_t *buf, uint32_t len_pixels)
{
    if (!display_valid(disp) || buf == NULL ||
        disp->type != RTK_DISPLAY_TYPE_SDI)
    {
        return -1;
    }

    if (len_pixels < (uint32_t)w * h)
    {
        return -1;
    }

    if (disp->pending)
    {
        int ret = rtk_display_wait_complete(disp, RTK_DISPLAY_WAIT_FOREVER);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (rtk_display_irq_prepare() != 0)
    {
        return -1;
    }

    rtk_sdi_set_TE_type(disp->u.sdi, LCDC_TE_TYPE_NO_TE);
    if (rtk_sdi_set_window(disp->u.sdi, x, y, w, h) != 0)
    {
        return -1;
    }
    if (rtk_sdi_start_transfer(disp->u.sdi, (uint8_t *)buf, len_pixels) != 0)
    {
        return -1;
    }

    disp->pending = true;
    return rtk_display_wait_complete(disp, RTK_DISPLAY_WAIT_FOREVER);
}

int rtk_display_commit(rtk_display_t *disp, const uint8_t *buf,
                       uint32_t len_pixels)
{
    int ret = rtk_display_submit(disp, buf, len_pixels,
                                 RTK_DISPLAY_WAIT_FOREVER);
    if (ret != 0)
    {
        return ret;
    }
    return rtk_display_wait_complete(disp, RTK_DISPLAY_WAIT_FOREVER);
}

bool rtk_display_power_on(rtk_display_t *disp)
{
    if (!display_valid(disp))
    {
        return false;
    }
    return (disp->type == RTK_DISPLAY_TYPE_SDI) ?
           rtk_sdi_power_on(disp->u.sdi) : rtk_rgb_power_on(disp->u.rgb);
}

bool rtk_display_power_off(rtk_display_t *disp)
{
    if (!display_valid(disp))
    {
        return false;
    }
    return (disp->type == RTK_DISPLAY_TYPE_SDI) ?
           rtk_sdi_power_off(disp->u.sdi) : rtk_rgb_power_off(disp->u.rgb);
}
