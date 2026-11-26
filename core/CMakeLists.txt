# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Panel-agnostic SDI/RGB controllers.  The controllers are SoC specific
# (LCDC/DBIC/eDPI register model), the panel data is not.
if(CONFIG_SOC_SERIES_RTL87X3G)
    target_sources(${LIBRARY_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/rtk_sdi_8773g.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtk_rgb_8773g.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtk_display.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rtk_display_irq.c)
endif()

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
