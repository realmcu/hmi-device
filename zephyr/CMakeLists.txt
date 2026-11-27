# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Touch controllers and their binding now belong to input/touch.
if(CONFIG_REALTEK_DISPLAY)
    add_subdirectory(display)
endif()
if(CONFIG_REALTEK_KEY_BUTTON_8773G_ZEPHYR)
    add_subdirectory(key_button/8773G)
endif()
