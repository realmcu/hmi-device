/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * SDK contract from legacy/8762G/lcd/EK9716_800480_rgb.c:148-161:
 * PLL1=100MHz, source selectors 0/1, divider selector 0. Only this complete
 * legacy configuration is supported. PLL2=160MHz/XTAL=40MHz comments alone
 * do not establish validated divider/enable sequences, so reject those rates.
 */
#include <stddef.h>
#include "rtk_display_soc.h"
#include "rtl_pinmux.h"
#include "rtl_rcc.h"
#include "utils.h"

#ifndef RTK_DISPLAY_SOC_IRQ_INSTALL
#include "rtl_nvic.h"
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
    if (clock != RTK_DISPLAY_CLK_100M)
    {
        return -1;
    }
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_ck_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_func_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_mux_clk_cg_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel0 = 0;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel1 = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_sel = 0;
    return 0;
}

void rtk_display_soc_pad_bus(uint32_t pin, bool rgb, bool pull_up)
{
    if (pin == RTK_DISPLAY_PIN_NONE)
    {
        return;
    }
    /* Both the EK9716 RGB and CO5300 QSPI/TE legacy paths use PINMUX mode
     * before enabling the dedicated pad. Unlike E/G, DBIC is not SW mode. */
    (void)rgb;
    Pad_Config(pin, PAD_PINMUX_MODE, PAD_IS_PWRON,
               pull_up ? PAD_PULL_UP : PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    /* Declared explicitly by the old EK9716 driver; older 2G SDKs may omit
     * this prototype from rtl_pinmux.h. Function name/signature is unchanged. */
    extern void Pad_Dedicated_Config(uint8_t Pin_Num, FunctionalState Status);
    Pad_Dedicated_Config(pin, ENABLE);
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
        /* Do not invent a dedicated-pad-disable sequence: the available 2G
         * legacy source only demonstrates dedicated pad ENABLE. Confirm
         * electrical sleep requirements with the actual SDK/board. */
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
    /* Display_IRQn and rtl_nvic.h occur in 2G legacy LCD sources. Vector-table
     * installation and DISPLAY_VECTORn are explicit SDK integration contracts,
     * not inferred validated vector numbers. Override with a real SDK hook if
     * the SDK installs ISRs differently; missing contracts fail compilation. */
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
    /* The available 2G DBIC driver initializes LCDC config to zero and does
     * not set PhaseShift. Preserve that instead of copying G's value. */
    return 0;
}
