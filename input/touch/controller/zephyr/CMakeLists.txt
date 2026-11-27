# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

set(touch_driver_sources "")
foreach(chip CHSC6417 CHSC6413 CST816D CST820)
    if(CONFIG_REALTEK_TOUCH_${chip}_ZEPHYR)
        if(chip MATCHES "^CST")
            string(TOLOWER "${chip}" filename)
        else()
            set(filename "${chip}")
        endif()
        list(APPEND touch_driver_sources
            ${CMAKE_CURRENT_SOURCE_DIR}/touch_${filename}_zephyr.c)
    endif()
endforeach()

list(LENGTH touch_driver_sources touch_driver_count)
if(touch_driver_count GREATER 1)
    message(FATAL_ERROR "Select only one Realtek Zephyr touch controller")
endif()
if(touch_driver_sources)
    target_sources(${LIBRARY_NAME} PRIVATE ${touch_driver_sources})
    # Preserve existing applications that include a chip-specific header.
    target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
