# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

# Board-level LCD bus data lives in the display module so every application on
# the same board shares one pin/clock definition.
if(CONFIG_BOARD_RTL87X3G_SAMPLE OR CONFIG_BOARD_RTL87X3G_EVB)
    target_sources(${LIBRARY_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/rtl87x3g/board_display.c)
    target_include_directories(${LIBRARY_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/rtl87x3g)
endif()
