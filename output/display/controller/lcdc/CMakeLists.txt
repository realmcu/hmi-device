# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

# One LCDC register-model implementation is shared by 8762G/8773E/8773G.
target_sources(${LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rtk_sdi_lcdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rtk_rgb_lcdc.c)
