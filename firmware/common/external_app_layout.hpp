/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef EXTERNAL_APP_LAYOUT_HPP
#define EXTERNAL_APP_LAYOUT_HPP

#include <cstdint>

namespace external_app {

constexpr uint32_t standard_version = 3;
constexpr uint32_t extended_version = 4;
constexpr uint32_t standard_base = 0x10080000;
constexpr uint32_t standard_size = 40 * 1024;
constexpr uint32_t extended_base = 0x10000000;
constexpr uint32_t extended_code_size = 88 * 1024;
constexpr uint32_t extended_ui_base = 0x10088000;
constexpr uint32_t extended_ui_size = 8 * 1024;
constexpr uint32_t header_size = 84;

constexpr bool supported(uint32_t version) {
    return version == standard_version || version == extended_version;
}

struct Layout {
    uint32_t ui_base = 0;
    uint32_t ui_size = 0;
    uint32_t m4_base = 0;
    uint32_t m4_size = 0;
};

// File order is UI, optional M4 image, trailing 32-bit checksum.

inline bool layout(uint32_t version, uint64_t file_size, uint32_t m4_offset, Layout& result, uint32_t standard_capacity = standard_size) {
    if (!supported(version) || file_size < header_size + 4 ||
        file_size > extended_code_size + extended_ui_size + 4 || (file_size & 3) || (m4_offset & 3))
        return false;

    const uint32_t payload_size = file_size - 4;

    if (m4_offset && (m4_offset < header_size || m4_offset >= payload_size))
        return false;

    result.ui_size = m4_offset ? m4_offset : payload_size;
    result.m4_size = m4_offset ? payload_size - m4_offset : 0;

    if (version == extended_version) {
        if (!result.m4_size || result.m4_size > extended_code_size || result.ui_size > extended_ui_size)
            return false;

        result.ui_base = extended_ui_base;
        result.m4_base = extended_base;
    } else {
        if (payload_size > standard_capacity)
            return false;

        result.ui_base = standard_base + result.m4_size;
        result.m4_base = standard_base;
    }

    return true;
}
}  // namespace external_app

#endif
