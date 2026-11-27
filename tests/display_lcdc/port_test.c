/* Fake HAL behavior/dispatch test. NOT a physical HAL/SDK test. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "fake_hal.h"
#include "rtk_display_soc.h"

FAKE_CLK fake_clk;
static unsigned pads, hs_funcs, hs_muxes, dedicated, clocks, rccs, configs, vectors, nvics, ms, us,
       hooks;
static uint32_t pad_pin;
static int pad_args[5], last_clock;
static void (*installed)(void);
static NVIC_InitTypeDef nvic_state;
void Pad_Config(uint32_t pin, int mode, int power, int pull, int output, int high) { ++pads; pad_pin = pin; pad_args[0] = mode; pad_args[1] = power; pad_args[2] = pull; pad_args[3] = output; pad_args[4] = high; }
void Pad_HighSpeedFuncSel(uint32_t pin, int fn) { ++hs_funcs; assert(pin == 7 && fn == HS_Func0); }
void Pad_HighSpeedMuxSel(uint32_t pin, int domain) { ++hs_muxes; assert(pin == 7 && domain == FROM_CORE_DOMAIN); }
void Pad_Dedicated_Config(uint8_t pin, FunctionalState state) { ++dedicated; assert(pin == 7 && state == ENABLE); }
void RCC_PeriphClockCmd(int p, int c, int on) { ++rccs; assert(p == APBPeriph_DISP && c == APBPeriph_DISP_CLOCK && on == ENABLE); }
void RCC_DisplayClockConfig(int source, int div) { ++configs; assert(source == DISPLAY_CLOCK_SOURCE_PLL1 && div == DISPLAY_CLOCK_DIV_1); }
uint32_t LCDC_Clock_Sel(LCDC_BUS_CLOCK clock) { ++clocks; last_clock = clock; return 0; }
void RamVectorTableUpdate(int vector, void(*handler)(void)) { ++vectors; assert(vector == DISPLAY_VECTORn); installed = handler; }
void NVIC_Init(NVIC_InitTypeDef *p) { ++nvics; nvic_state = *p; }
void platform_delay_ms(uint32_t delay) { ms = delay; }
void platform_delay_us(uint32_t delay) { us = delay; }
static void handler(void) { }
int fake_irq_install(void(*callback)(void)) { ++hooks; installed = callback; return -37; }

int main(void)
{
    assert(RTK_DISPLAY_CLK_200M == 0 && RTK_DISPLAY_CLK_280M == 1);
    const T_RTK_DISPLAY_CLK rates[] = {RTK_DISPLAY_CLK_200M, RTK_DISPLAY_CLK_280M, RTK_DISPLAY_CLK_40M, RTK_DISPLAY_CLK_100M, RTK_DISPLAY_CLK_160M, (T_RTK_DISPLAY_CLK)99};
    for (unsigned i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i)
    {
        unsigned before = clocks + rccs + configs;
        T_RTK_DISPLAY_CLK rate = rates[i];
#if TEST_SOC == 3
        bool supported = rate == RTK_DISPLAY_CLK_200M || rate == RTK_DISPLAY_CLK_280M;
#elif TEST_SOC == 2
        bool supported = rate == RTK_DISPLAY_CLK_200M;
#else
        bool supported = rate == RTK_DISPLAY_CLK_100M;
#endif
        assert((rtk_display_soc_clock_config(rate) == 0) == supported);
        if (!supported) { assert(before == clocks + rccs + configs); }
    }
#if TEST_SOC == 3
    assert(clocks == 2 && last_clock == LCDC_BUS_CLK_280M && rccs == 0 && configs == 0);
#elif TEST_SOC == 2
    assert(clocks == 0 && rccs == 1 && configs == 1);
#else
    assert(clocks == 0 && rccs == 1 && configs == 0);
    assert(fake_clk.u_324.BITS_324.disp_ck_en == 1 && fake_clk.u_324.BITS_324.disp_func_en == 1);
    assert(fake_clk.u_324.BITS_324.r_disp_mux_clk_cg_en == 1 &&
           fake_clk.u_324.BITS_324.r_disp_div_en == 1);
    assert(fake_clk.u_324.BITS_324.r_disp_clk_src_sel0 == 0 &&
           fake_clk.u_324.BITS_324.r_disp_clk_src_sel1 == 1 && fake_clk.u_324.BITS_324.r_disp_div_sel == 0);
#endif
    rtk_display_soc_pad_bus(RTK_DISPLAY_PIN_NONE, true, true);
    rtk_display_soc_pad_output(RTK_DISPLAY_PIN_NONE, true);
    rtk_display_soc_pad_sleep(RTK_DISPLAY_PIN_NONE);
    assert(pads + hs_funcs + hs_muxes + dedicated == 0);
    rtk_display_soc_pad_bus(7, true, true);
    assert(pads == 1 && pad_pin == 7 && pad_args[0] == PAD_PINMUX_MODE && pad_args[1] == PAD_IS_PWRON &&
           pad_args[2] == PAD_PULL_UP && pad_args[3] == PAD_OUT_DISABLE && pad_args[4] == PAD_OUT_HIGH);
    rtk_display_soc_pad_bus(7, false, false);
    assert(pads == 2 && pad_args[0] == (TEST_SOC == 1 ? PAD_PINMUX_MODE : PAD_SW_MODE) &&
           pad_args[2] == PAD_PULL_DOWN);
#if TEST_SOC == 1
    assert(dedicated == 2 && hs_funcs == 0 && hs_muxes == 0);
#else
    assert(dedicated == 0 && hs_funcs == 2 && hs_muxes == 2);
#endif
    rtk_display_soc_pad_output(7, true);
    assert(pads == 3 && pad_args[0] == PAD_SW_MODE && pad_args[2] == PAD_PULL_NONE &&
           pad_args[3] == PAD_OUT_ENABLE && pad_args[4] == PAD_OUT_HIGH);
    rtk_display_soc_pad_output(7, false);
    assert(pads == 4 && pad_args[4] == PAD_OUT_LOW);
    rtk_display_soc_pad_sleep(7);
    assert(pads == 5 && pad_args[0] == PAD_SW_MODE && pad_args[2] == PAD_PULL_DOWN &&
           pad_args[3] == PAD_OUT_DISABLE && pad_args[4] == PAD_OUT_LOW);
#if TEST_SOC == 1
    assert(hs_muxes == 0 && dedicated == 2);
#else
    assert(hs_muxes == 3);
#endif
    assert(rtk_display_soc_irq_install(NULL) == -1 && vectors + nvics + hooks == 0);
#ifdef TEST_IRQ_HOOK
    assert(rtk_display_soc_irq_install(handler) == -37 && hooks == 1 && vectors == 0 && nvics == 0 &&
           installed == handler);
#else
    assert(rtk_display_soc_irq_install(handler) == 0 && vectors == 1 && nvics == 1 &&
           installed == handler);
    assert(nvic_state.NVIC_IRQChannel == Display_IRQn && nvic_state.NVIC_IRQChannelPriority == 3 &&
           nvic_state.NVIC_IRQChannelCmd == ENABLE);
#endif
    rtk_display_soc_delay_ms(123); rtk_display_soc_delay_us(456);
    assert(ms == 123 && us == 456);
    assert(rtk_display_soc_sdi_phase_shift() == (TEST_SOC == 1 ? 0 : 2));
    puts("PASS fake HAL SoC port clock/pad/IRQ/delay/phase dispatch");
    return 0;
}
