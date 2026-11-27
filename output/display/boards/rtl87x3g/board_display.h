/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef BOARD_DISPLAY_H
#define BOARD_DISPLAY_H

#include "rtk_sdi.h"
#include "rtk_rgb.h"
#include "rtk_display.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board-level bus data for rtl87x3g sample/EVB.  Panel data files contain no pins. */
extern const T_RTK_SDI_BUS board_sdi_sh8601_bus;
extern const T_RTK_SDI_BUS board_sdi_st7801n_bus;
extern const T_RTK_SDI_BUS board_sdi_auto_360_bus;
extern const T_RTK_RGB_BUS board_rgb_bus;

/* Application-facing default display.  Change this function (or its body) to
 * select a different panel; gui_port_dc.c stays unchanged. */
int board_display_new(rtk_display_t **out);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_DISPLAY_H */
