/*
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __METADATA_FILE_HPP__
#define __METADATA_FILE_HPP__

#include "file.hpp"
#include "optional.hpp"

struct capture_metadata {
    uint64_t center_frequency;
    uint32_t sample_rate;
};

std::filesystem::path get_metadata_path(const std::filesystem::path& capture_path);

bool write_metadata_file(const std::filesystem::path& path, capture_metadata metadata);
Optional<capture_metadata> read_metadata_file(const std::filesystem::path& path);

#endif  // __METADATA_FILE_HPP__