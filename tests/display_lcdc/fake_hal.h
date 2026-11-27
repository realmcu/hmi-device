/* Fake SDK for dispatch tests only; never use for physical hardware. */
#ifndef DISPLAY_TEST_FAKE_HAL_H
#define DISPLAY_TEST_FAKE_HAL_H
#include <stdint.h>
#include <stddef.h>
#define ENABLE 1
#define DISABLE 0
#define PAD_PINMUX_MODE 10
#define PAD_SW_MODE 11
#define PAD_IS_PWRON 12
#define PAD_PULL_UP 13
#define PAD_PULL_DOWN 14
#define PAD_PULL_NONE 15
#define PAD_OUT_DISABLE 16
#define PAD_OUT_ENABLE 17
#define PAD_OUT_HIGH 18
#define PAD_OUT_LOW 19
#define HS_Func0 20
#define FROM_CORE_DOMAIN 21
#define APBPeriph_DISP 22
#define APBPeriph_DISP_CLOCK 23
#define DISPLAY_CLOCK_SOURCE_PLL1 24
#define DISPLAY_CLOCK_DIV_1 25
#define DISPLAY_VECTORn 26
#define Display_IRQn 27
typedef int LCDC_BUS_CLOCK;
#define LCDC_BUS_CLK_200M 28
#define LCDC_BUS_CLK_280M 29
typedef struct { unsigned NVIC_IRQChannel, NVIC_IRQChannelPriority, NVIC_IRQChannelCmd; } NVIC_InitTypeDef;
typedef struct { struct { unsigned disp_ck_en, disp_func_en, r_disp_mux_clk_cg_en, r_disp_div_en, r_disp_clk_src_sel0, r_disp_clk_src_sel1, r_disp_div_sel; } BITS_324; } FAKE_CLK_WORD;
typedef struct { FAKE_CLK_WORD u_324; } FAKE_CLK;
extern FAKE_CLK fake_clk;
#define PERIBLKCTRL_PERI_CLK (&fake_clk)
void Pad_Config(uint32_t, int, int, int, int, int);
void Pad_HighSpeedFuncSel(uint32_t, int);
void Pad_HighSpeedMuxSel(uint32_t, int);
typedef int FunctionalState;
void Pad_Dedicated_Config(uint8_t, FunctionalState);
void RCC_PeriphClockCmd(int, int, int);
void RCC_DisplayClockConfig(int, int);
uint32_t LCDC_Clock_Sel(LCDC_BUS_CLOCK);
void RamVectorTableUpdate(int, void(*)(void));
void NVIC_Init(NVIC_InitTypeDef *);
void platform_delay_ms(uint32_t);
void platform_delay_us(uint32_t);
#endif
