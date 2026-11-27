/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include "rtk_display_soc.h"
#include "rtl_lcdc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "platform_utils.h"
#include "vector_table.h"

int rtk_display_soc_clock_config(T_RTK_DISPLAY_CLK clock)
{
    if (clock == RTK_DISPLAY_CLK_200M)
    {
        (void)LCDC_Clock_Sel(LCDC_BUS_CLK_200M);
    }
    else if (clock == RTK_DISPLAY_CLK_280M)
    {
        (void)LCDC_Clock_Sel(LCDC_BUS_CLK_280M);
    }
    else
    {
        return -1;
    }
    /* Keep the established G clock-hook behavior: LCDC_Clock_Sel's reported
     * actual MHz was not used as an error code by the original controllers. */
    return 0;
}

void rtk_display_soc_pad_bus(uint32_t pin, bool rgb, bool pull_up)
{
    if (pin == RTK_DISPLAY_PIN_NONE)
    {
        return;
    }
    Pad_Config(pin, rgb ? PAD_PINMUX_MODE : PAD_SW_MODE, PAD_IS_PWRON,
               pull_up ? PAD_PULL_UP : PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_HighSpeedFuncSel(pin, HS_Func0);
    Pad_HighSpeedMuxSel(pin, FROM_CORE_DOMAIN);
}

void rtk_display_soc_pad_output(uint32_t pin, bool high)
{
    if (pin != RTK_DISPLAY_PIN_NONE)
    {
        Pad_Config(pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE,
                   PAD_OUT_ENABLE, high ? PAD_OUT_HIGH : PAD_OUT_LOW);
    }
}

void rtk_display_soc_pad_sleep(uint32_t pin)
{
    if (pin != RTK_DISPLAY_PIN_NONE)
    {
        Pad_Config(pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN,
                   PAD_OUT_DISABLE, PAD_OUT_LOW);
        Pad_HighSpeedMuxSel(pin, FROM_CORE_DOMAIN);
    }
}

int rtk_display_soc_irq_install(void (*handler)(void))
{
    if (handler == NULL)
    {
        return -1;
    }
    RamVectorTableUpdate(DISPLAY_VECTORn, handler);
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel = Display_IRQn;
    nvic.NVIC_IRQChannelPriority = 3;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    return 0;
}

void rtk_display_soc_delay_ms(uint32_t delay)
{
    platform_delay_ms(delay);
}

void rtk_display_soc_delay_us(uint32_t delay)
{
    platform_delay_us(delay);
}

uint8_t rtk_display_soc_sdi_phase_shift(void)
{
    return 2;
}
