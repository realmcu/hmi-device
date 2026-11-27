# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Input devices do not require the display module or its hardware library.
if(NOT CONFIG_REALTEK_DISPLAY AND NOT CONFIG_REALTEK_TOUCH AND
   NOT CONFIG_REALTEK_KEY_BUTTON)
    return()
endif()

set(HW_DEVICE_TARGET_NAME realtek_device_driver_library)
set(LIBRARY_NAME ${HW_DEVICE_TARGET_NAME})

if(ZEPHYR_BASE)
    zephyr_library_named(${LIBRARY_NAME})
else()
    add_library(${LIBRARY_NAME} STATIC)
endif()

# Only display devices consume display controller headers and implementations.
# Declare the target dependency even if that module is processed later.
if(CONFIG_REALTEK_DISPLAY)
    target_link_libraries(${LIBRARY_NAME} PUBLIC realtek_display_driver_library)
    add_subdirectory(output/display)
endif()

add_subdirectory(legacy)
add_subdirectory(input)

if(ZEPHYR_BASE)
    add_subdirectory(zephyr)
endif()

if(CONFIG_REALTEK_DISPLAY AND CONFIG_REALTEK_DISPLAY_GUI_ACC AND TARGET guiLib)
    target_link_libraries(${LIBRARY_NAME} PRIVATE guiLib)
endif()

# The pure touch core needs no HAL. Display and legacy input drivers do.
if(NOT ZEPHYR_BASE AND (CONFIG_REALTEK_DISPLAY OR CONFIG_REALTEK_KEY_BUTTON OR
   CONFIG_REALTEK_TOUCH_CHSC6417 OR CONFIG_REALTEK_TOUCH_CHSC6413 OR
   CONFIG_REALTEK_TOUCH_GT911 OR CONFIG_REALTEK_TOUCH_CST816D OR
   CONFIG_REALTEK_TOUCH_CST820))
    target_link_libraries(${LIBRARY_NAME} PUBLIC halLib)
endif()
