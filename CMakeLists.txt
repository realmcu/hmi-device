# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

if(NOT CONFIG_REALTEK_DISPLAY)
    return()
endif()

set(HW_DEVICE_TARGET_NAME realtek_device_driver_library)
set(LIBRARY_NAME ${HW_DEVICE_TARGET_NAME})
# Device subdirectories historically use the display module's target variable.
set(HW_DISPLAY_TARGET_NAME ${HW_DEVICE_TARGET_NAME})

if(ZEPHYR_BASE)
    zephyr_library_named(${LIBRARY_NAME})
else()
    add_library(${LIBRARY_NAME} STATIC)
endif()

# Device drivers use display hardware headers/includes.  The display module is
# expected to be processed first (manifest order); guard for non-display builds.
if(TARGET realtek_display_driver_library)
    target_link_libraries(${LIBRARY_NAME} PUBLIC realtek_display_driver_library)
endif()

add_subdirectory(legacy)
add_subdirectory(output/display)

if(ZEPHYR_BASE)
    add_subdirectory(zephyr)
endif()

if(CONFIG_REALTEK_DISPLAY_GUI_ACC AND TARGET guiLib)
    target_link_libraries(${LIBRARY_NAME} PRIVATE guiLib)
endif()

if(NOT ZEPHYR_BASE)
    target_link_libraries(${LIBRARY_NAME} PUBLIC halLib)
endif()
