/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * SDK contract: legacy/8773E/lcd/lcd_st7265_800480_rgb.c configures
 * RCC_DisplayClockConfig(PLL1, DIV_1) and explicitly documents PLL1:200M.
 * Only this source/rate is supported; do not assume the G PLL2 rate on E.
 */
#include <stddef.h>
#include "rtk_display_soc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "platform_utils.h"

/* An external SDK may replace the entire IRQ installation contract. */
#ifndef RTK_DISPLAY_SOC_IRQ_INSTALL
#include "rtl876x_nvic.h"
#include "vector_table.h"
#ifndef RTK_DISPLAY_SOC_IRQ_VECTOR
#define RTK_DISPLAY_SOC_IRQ_VECTOR DISPLAY_VECTORn
#endif
#ifndef RTK_DISPLAY_SOC_IRQ_NUMBER
#define RTK_DISPLAY_SOC_IRQ_NUMBER Display_IRQn
#endif
#endif

int rtk_display_soc_clock_config(T_RTK_DISPLAY_CLK clock)
{
    if (clock != RTK_DISPLAY_CLK_200M)
    {
        return -1;
    }
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);
    RCC_DisplayClockConfig(DISPLAY_CLOCK_SOURCE_PLL1, DISPLAY_CLOCK_DIV_1);
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
#ifdef RTK_DISPLAY_SOC_IRQ_INSTALL
    return RTK_DISPLAY_SOC_IRQ_INSTALL(handler);
#else
    /* E legacy drivers use the rtl876x NVIC/vector-table SDK family. The
     * display vector names below are an integration contract, not a claim
     * that this checkout contains or has validated the E interrupt table.
     * Missing names must fail compilation or be overridden for the real SDK. */
    RamVectorTableUpdate(RTK_DISPLAY_SOC_IRQ_VECTOR, handler);
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel = RTK_DISPLAY_SOC_IRQ_NUMBER;
    nvic.NVIC_IRQChannelPriority = 3;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    return 0;
#endif
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
    /* legacy/8773E/lcd/lcd_sh8601z_410_502_qspi.c sets PhaseShift=0x2. */
    return 2;
}
