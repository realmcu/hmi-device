/* Execute shared init's clock-failure path without touching LCD MMIO. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "rtk_rgb.h"
#include "rtk_sdi.h"
#include "rtk_display_soc.h"
static unsigned clock_calls;
int rtk_display_soc_clock_config(T_RTK_DISPLAY_CLK clock) { assert(clock == (T_RTK_DISPLAY_CLK)99); ++clock_calls; return -1; }
void rtk_display_soc_pad_bus(uint32_t pin, bool rgb, bool pull) { (void)pin; (void)rgb; (void)pull; }
void rtk_display_soc_pad_output(uint32_t pin, bool high) { (void)pin; (void)high; }
void rtk_display_soc_pad_sleep(uint32_t pin) { (void)pin; }
void rtk_display_soc_delay_ms(uint32_t delay) { (void)delay; assert(!"unexpected delay"); }
void rtk_display_soc_delay_us(uint32_t delay) { (void)delay; assert(!"unexpected delay"); }
uint8_t rtk_display_soc_sdi_phase_shift(void) { assert(!"unexpected phase access"); return 0; }
int rtk_display_soc_irq_install(void(*handler)(void)) { (void)handler; assert(!"unexpected IRQ setup"); return -1; }
int main(void)
{
    T_RTK_RGB_PANEL rgb_panel = {0};
    rgb_panel.width = 2; rgb_panel.height = 2; rgb_panel.input_bits = 16; rgb_panel.output_bits = 16;
    T_RTK_RGB_BUS rgb_bus;
    memset(&rgb_bus, 0xff, sizeof(rgb_bus)); rgb_bus.clock = (T_RTK_DISPLAY_CLK)99;
    rtk_rgb_t *rgb = NULL;
    assert(rtk_rgb_new_panel(&rgb_panel, &rgb_bus, &rgb) == 0);
    assert(rtk_rgb_init(rgb) == -1 && clock_calls == 1);
    assert(rtk_rgb_init(rgb) == -1 && clock_calls == 2);
    T_RTK_SDI_PANEL sdi_panel = {0};
    T_RTK_SDI_BUS sdi_bus;
    memset(&sdi_bus, 0xff, sizeof(sdi_bus)); sdi_bus.clock = (T_RTK_DISPLAY_CLK)99;
    rtk_sdi_t *sdi = NULL;
    assert(rtk_sdi_new_panel(&sdi_panel, &sdi_bus, &sdi) == 0);
    assert(rtk_sdi_init(sdi) == -1 && clock_calls == 3);
    assert(rtk_sdi_init(sdi) == -1 && clock_calls == 4);
    puts("PASS shared RGB/SDI init rejects clock error before LCD MMIO/configuration");
    return 0;
}
