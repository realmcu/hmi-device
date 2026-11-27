# Copyright (c) 2026, Realtek Semiconductor Corporation. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

if(NOT ZEPHYR_BASE AND (CONFIG_REALTEK_TOUCH_CHSC6417_ZEPHYR OR
   CONFIG_REALTEK_TOUCH_CHSC6413_ZEPHYR OR CONFIG_REALTEK_TOUCH_CST816D_ZEPHYR OR
   CONFIG_REALTEK_TOUCH_CST820_ZEPHYR))
    message(FATAL_ERROR "Realtek _ZEPHYR touch controllers require a Zephyr build")
endif()

add_subdirectory(core)

if(ZEPHYR_BASE)
    add_subdirectory(ports/zephyr)
    add_subdirectory(controller/zephyr)
endif()
