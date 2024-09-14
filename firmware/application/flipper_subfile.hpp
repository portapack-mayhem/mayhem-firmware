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

struct flippersub_metadata {
    rf::Frequency center_frequency;
    float latitude = 0;
    float longitude = 0;
    uint8_t protocol = 0;
    uint16_t te = 0;
};

Optional<flippersub_metadata> read_flippersub_file(const std::filesystem::path& path);

// Maybe sometime there will be a data part reader / converter

#endif  // __FLIPPER_SUBFILE_HPP__