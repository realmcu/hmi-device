# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Preserve the existing 8773G default for non-Kconfig CMake consumers.
if(NOT DEFINED CONFIG_REALTEK_DISPLAY_LCDC_SHARED)
    if(CONFIG_SOC_SERIES_RTL87X3G OR CONFIG_SOC_SERIES_RTL8773G)
        set(CONFIG_REALTEK_DISPLAY_LCDC_SHARED ON)
    else()
        set(CONFIG_REALTEK_DISPLAY_LCDC_SHARED OFF)
    endif()
endif()
# Legacy source selection happens in the parent device directory.
set(CONFIG_REALTEK_DISPLAY_LCDC_SHARED ${CONFIG_REALTEK_DISPLAY_LCDC_SHARED} PARENT_SCOPE)

if(CONFIG_REALTEK_DISPLAY_LCDC_SHARED)
    add_subdirectory(ports)
    add_subdirectory(controller/lcdc)
    add_subdirectory(boards)
    add_subdirectory(core)
    add_subdirectory(panel)
endif()
