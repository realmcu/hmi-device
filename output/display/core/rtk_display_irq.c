/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtk_display_irq.h"

#include "os_sync.h"
#include "rtl_lcdc.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"

static void *display_sync_sem;
static bool irq_initialized;

static void rtk_display_irq_handler(void)
{
    if (LCDC_GetINTStatus(LCDC_STATUS_TX_AUTO_DONE_INT) == SET)
    {
        LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);

        if (display_sync_sem != NULL)
        {
            (void)os_sem_give(display_sync_sem);
        }
    }
}

int rtk_display_irq_init(void)
{
    if (irq_initialized)
    {
        return 0;
    }

    if (!os_sem_create(&display_sync_sem, "lcd_sync", 0, 1))
    {
        display_sync_sem = NULL;
        return -1;
    }

    RamVectorTableUpdate(DISPLAY_VECTORn, rtk_display_irq_handler);

    LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);
    LCDC_MaskINTConfig(LCDC_INT_MASK_TX_AUTO_DONE, DISABLE);

    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel = Display_IRQn;
    nvic.NVIC_IRQChannelPriority = 3;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    irq_initialized = true;
    return 0;
}

int rtk_display_irq_prepare(void)
{
    if (!irq_initialized || display_sync_sem == NULL)
    {
        return -1;
    }

    /* Drop a stale give from a timed-out previous wait. */
    while (os_sem_take(display_sync_sem, 0))
    {
    }

    LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);
    return 0;
}

int rtk_display_irq_wait(uint32_t timeout_ms)
{
    if (!irq_initialized || display_sync_sem == NULL)
    {
        return -1;
    }

    return os_sem_take(display_sync_sem, timeout_ms) ? 0 : -1;
}
