/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Types shared by the SDI and RGB display controllers.
 */

#ifndef RTK_DISPLAY_TYPES_H
#define RTK_DISPLAY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    RTK_DISPLAY_CLK_200M = 0,
    RTK_DISPLAY_CLK_280M,
} T_RTK_DISPLAY_CLK;

#define RTK_DISPLAY_PIN_NONE 0xFFFFFFFFu

#ifdef __cplusplus
}
#endif

#endif /* RTK_DISPLAY_TYPES_H */
