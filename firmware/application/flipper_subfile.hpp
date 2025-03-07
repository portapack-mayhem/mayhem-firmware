/*
 * Copyright (C) 2024 HTotoo
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

#ifndef __FLIPPER_SUBFILE_HPP__
#define __FLIPPER_SUBFILE_HPP__

#include "metadata_file.hpp"

typedef enum : uint8_t {
    FLIPPER_PROTO_UNSUPPORTED = 0,
    FLIPPER_PROTO_RAW = 1,
    FLIPPER_PROTO_BINRAW = 2
} FlipperProto;

typedef enum : uint8_t {
    FLIPPER_PRESET_UNK = 0,
    FLIPPER_PRESET_CUSTOM = 1,
    FLIPPER_PRESET_OOK = 2,
    FLIPPER_PRESET_2FSK = 3,
} FlipperPreset;

struct flippersub_metadata {
    rf::Frequency center_frequency;
    float latitude = 0;
    float longitude = 0;
    FlipperProto protocol = FLIPPER_PROTO_UNSUPPORTED;
    FlipperPreset preset = FLIPPER_PRESET_UNK;
    uint16_t te = 0;
    uint32_t binraw_bit_count = 0;
};

Optional<flippersub_metadata> read_flippersub_file(const std::filesystem::path& path);
bool seek_flipper_raw_first_data(File& f);
bool seek_flipper_binraw_first_data(File& f, bool seekzero = true);
Optional<int32_t> read_flipper_raw_next_data(File& f);
Optional<uint8_t> read_flipper_binraw_next_data(File& f);
bool get_flipper_binraw_bitvalue(uint8_t byte, uint8_t nthBit);

// Maybe sometime there will be a data part reader / converter

#endif  // __FLIPPER_SUBFILE_HPP__