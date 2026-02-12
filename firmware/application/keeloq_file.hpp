/*
 * Copyright (C) 2026 lifegame1lu111
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

#ifndef __KEELOQ_FILE__
#define __KEELOQ_FILE__

#include "portapack.hpp"
#include "file_reader.hpp"
#include "string_format.hpp"
#include <string_view>

struct KeeloqData {
    std::string mf_name{};
    uint32_t serial = 0;
    uint16_t counter = 0;
    uint8_t btn = 0;
};

bool read_keeloq_file(const std::filesystem::path&, KeeloqData&);
bool write_keeloq_file(const std::filesystem::path&, const KeeloqData&);

#endif
