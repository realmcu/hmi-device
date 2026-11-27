# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

target_sources(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/rtk_touch.c)
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
