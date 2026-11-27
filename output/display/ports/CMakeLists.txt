# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

set(rtk_display_soc_backends "")
if(CONFIG_SOC_SERIES_RTL87X3G OR CONFIG_SOC_SERIES_RTL8773G)
    list(APPEND rtk_display_soc_backends rtl8773g.c)
endif()
if(CONFIG_SOC_SERIES_RTL8773E OR CONFIG_SOC_SERIES_RTL87X3E)
    list(APPEND rtk_display_soc_backends rtl8773e.c)
endif()
if(CONFIG_SOC_SERIES_RTL87X2G)
    list(APPEND rtk_display_soc_backends rtl8762g.c)
endif()

list(LENGTH rtk_display_soc_backends rtk_display_soc_count)
if(NOT rtk_display_soc_count EQUAL 1)
    message(FATAL_ERROR "Select exactly one supported display SoC: RTL8773G, RTL8773E or RTL8762G")
endif()
foreach(backend IN LISTS rtk_display_soc_backends)
    target_sources(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${backend})
endforeach()
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
