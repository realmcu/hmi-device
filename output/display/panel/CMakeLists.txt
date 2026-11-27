# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# The new panel-data + bus model is 8773G-only.
if(NOT CONFIG_SOC_SERIES_RTL87X3G)
    return()
endif()

# Panel list and "only one SDI/RGB panel" guards are generated from
# registry/panels.json.
include(${CMAKE_CURRENT_LIST_DIR}/../registry/panel_registry.cmake)

if(CONFIG_REALTEK_DISPLAY_USE_PANEL_MANIFEST)
    # Code-selected mode: compile every migrated SDI and RGB panel data file.
    # The application binds one with rtk_sdi_new_panel_*() or
    # rtk_rgb_new_panel_*(); the linker drops the unused objects.
    foreach(panel_entry IN LISTS RTK_8773G_PANEL_SOURCES)
        string(REPLACE "|" ";" panel_parts "${panel_entry}")
        list(GET panel_parts 1 panel_source)
        target_sources(${LIBRARY_NAME} PRIVATE ${panel_source})
    endforeach()
else()
    # Legacy per-panel Kconfig mode: build only the selected panel.
    foreach(panel_entry IN LISTS RTK_8773G_PANEL_SOURCES)
        string(REPLACE "|" ";" panel_parts "${panel_entry}")
        list(GET panel_parts 0 panel_config)
        list(GET panel_parts 1 panel_source)
        if(${panel_config})
            target_sources(${LIBRARY_NAME} PRIVATE ${panel_source})
        endif()
    endforeach()
endif()

target_include_directories(${LIBRARY_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sdi
    ${CMAKE_CURRENT_SOURCE_DIR}/rgb)
