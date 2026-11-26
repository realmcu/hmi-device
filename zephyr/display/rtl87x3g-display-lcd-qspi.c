/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr display driver -> rtk_display facade.
 *
 * Init timing:
 *   Zephyr runs this driver at POST_KERNEL, before main().  At that point
 *   app_system_lower_init() has NOT initialized PSRAM yet and the panel handle
 *   does not exist.  Hardware init is therefore deferred to the application:
 *   gui_port_dc_init() calls board_display_new() + rtk_display_init() after
 *   app_system_lower_init() has brought up PSRAM/clocks.
 *
 *   This wrapper only owns the Zephyr display device object; write/power/
 *   capabilities resolve rtk_display_get_default() at call time.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#include "rtk_display.h"

#define DT_DRV_COMPAT realtek_rtl87x3g_display_lcd

struct lcd_display_config
{
    mm_reg_t base;
    size_t mem_size;
    uint16_t width;
    uint16_t height;
    enum display_pixel_format pixel_format;
};

struct lcd_display_data
{
    void *framebuffer;
    struct display_capabilities capabilities;
    bool initialized;
};

static int lcd_display_hw_init(const struct device *dev);

static enum display_pixel_format pixel_format_from_bits(uint8_t pixel_bits)
{
    switch (pixel_bits)
    {
    case 16:
                return PIXEL_FORMAT_RGB_565;
    case 24:
        return PIXEL_FORMAT_RGB_888;
    case 32:
        return PIXEL_FORMAT_ARGB_8888;
    default:
        return PIXEL_FORMAT_RGB_565;
    }
}

static void lcd_display_get_capabilities(const struct device *dev,
                                         struct display_capabilities *caps)
{
    const struct lcd_display_config *config = dev->config;
    rtk_display_t *display = rtk_display_get_default();

    if (display != NULL)
    {
        caps->x_resolution = rtk_display_get_width(display);
        caps->y_resolution = rtk_display_get_height(display);
        caps->current_pixel_format =
            pixel_format_from_bits(rtk_display_get_pixel_bits(display));
    }
    else
    {
        /* POST_KERNEL: hardware is initialized later by the application. */
        caps->x_resolution = config->width;
        caps->y_resolution = config->height;
        caps->current_pixel_format = config->pixel_format;
    }

    caps->supported_pixel_formats = caps->current_pixel_format;
    caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
    caps->screen_info = 0;
}

static int lcd_display_blanking_on(const struct device *dev)
{
    struct lcd_display_data *data = dev->data;
    rtk_display_t *display = rtk_display_get_default();

    if (!data->initialized || display == NULL)
    {
        return -EAGAIN;
    }

    printk("Display Blanking ON (Power Off)\n");
    return rtk_display_power_off(display) ? 0 : -EIO;
}

static int lcd_display_blanking_off(const struct device *dev)
{
    struct lcd_display_data *data = dev->data;
    rtk_display_t *display = rtk_display_get_default();

    if (!data->initialized || display == NULL)
    {
        return -EAGAIN;
    }

    printk("Display Blanking OFF (Power On)\n");
    return rtk_display_power_on(display) ? 0 : -EIO;
}

static int lcd_display_write(const struct device *dev, const uint16_t x,
                             const uint16_t y,
                             const struct display_buffer_descriptor *desc,
                             const void *buf)
{
    struct lcd_display_data *data = dev->data;
    rtk_display_t *display = rtk_display_get_default();

    if (!data->initialized || display == NULL)
    {
        return -EAGAIN;
    }

    uint32_t width = rtk_display_get_width(display);
    uint32_t height = rtk_display_get_height(display);

    bool full_frame = (x == 0 && y == 0 &&
                       desc->width == width && desc->height == height);

    if (full_frame)
    {
        if (rtk_display_commit(display, buf, width * height) != 0)
        {
            return -EIO;
        }
        return 0;
    }

    /* PFB path.  RGB has no partial-window transfer; its caller must provide
     * a full-frame display_write(). */
    if (rtk_display_get_type(display) != RTK_DISPLAY_TYPE_SDI)
    {
        return -ENOTSUP;
    }

    if (rtk_display_write_region(display, x, y,
                                 desc->width, desc->height, buf,
                                 (uint32_t)desc->width * desc->height) != 0)
    {
        return -EIO;
    }
    return 0;
}

static void *lcd_display_get_framebuffer(const struct device *dev)
{
    return NULL;
}

static int lcd_display_set_brightness(const struct device *dev,
                                      uint8_t brightness)
{
    return -ENOSYS;
}

static int lcd_display_set_contrast(const struct device *dev, uint8_t contrast)
{
    return -ENOSYS;
}

static int lcd_display_set_pixel_format(const struct device *dev,
                                        const enum display_pixel_format pixel_format)
{
    return -ENOSYS;
}

static int lcd_display_set_orientation(const struct device *dev,
                                       const enum display_orientation orientation)
{
    return -ENOSYS;
}

static int lcd_display_init(const struct device *dev)
{
    const struct lcd_display_config *config = dev->config;
    struct lcd_display_data *data = dev->data;

    printk("Initializing Realtek LCD Display '%s'...\n", dev->name);
    printk("Display resolution: %dx%d\n", config->width, config->height);

    data->initialized = false;
    data->framebuffer = NULL;

    int ret = lcd_display_hw_init(dev);
    if (ret != 0)
    {
        printk("ERROR: LCD hardware initialization failed: %d\n", ret);
        return ret;
    }

    return 0;
}

static int lcd_display_hw_init(const struct device *dev)
{
    struct lcd_display_data *data = dev->data;

    if (data->initialized)
    {
        return 0;
    }

    /*
     * Do not touch the panel or PSRAM here.  This runs at POST_KERNEL before
     * main() and before app_system_lower_init() initializes PSRAM/clocks.
     * gui_port_dc_init() calls rtk_display_init() after lower init.
     */
    data->initialized = true;
    printk("LCD hardware init deferred to application after lower init\n");
    return 0;
}

static const struct display_driver_api lcd_display_api =
{
    .blanking_on = lcd_display_blanking_on,
    .blanking_off = lcd_display_blanking_off,
    .write = lcd_display_write,
    .read = NULL,
    .get_framebuffer = lcd_display_get_framebuffer,
    .set_brightness = lcd_display_set_brightness,
    .set_contrast = lcd_display_set_contrast,
    .get_capabilities = lcd_display_get_capabilities,
    .set_pixel_format = lcd_display_set_pixel_format,
    .set_orientation = lcd_display_set_orientation,
};

#define LCD_DISPLAY_INIT(index)                                             \
    static struct lcd_display_data lcd_display_data_##index;                \
    static const struct lcd_display_config lcd_display_cfg_##index = {      \
        .base = DT_INST_REG_ADDR(index),                                    \
                .mem_size = DT_INST_REG_SIZE(index),                                \
                            .width = DT_INST_PROP(index, width),                                \
                                     .height = DT_INST_PROP(index, height),                              \
                                               .pixel_format = PIXEL_FORMAT_RGB_565,                               \
    };                                                                      \
    DEVICE_DT_INST_DEFINE(index,                                            \
                          lcd_display_init,                                 \
                          NULL,                                             \
                          &lcd_display_data_##index,                        \
                          &lcd_display_cfg_##index,                         \
                          POST_KERNEL,                                      \
                          CONFIG_DISPLAY_INIT_PRIORITY,                     \
                          &lcd_display_api);

DT_INST_FOREACH_STATUS_OKAY(LCD_DISPLAY_INIT)
