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

#include "keeloq_file.hpp"

namespace fs = std::filesystem;

bool read_keeloq_file(const fs::path& path, KeeloqData& data) {
    File file{};

    auto result = file.open(path);

    if (result) {
        return false;
    }

    FileLineReader reader{file};

    std::string raw = *reader.begin();
    auto chunks = split_string(raw, ';');

    if (chunks.size() != 4) {
        return false;
    }

    data.mf_name = std::string{chunks[0]};
    data.serial = std::strtoul(chunks[1].data(), NULL, 16);
    data.counter = (uint16_t)std::atoi(chunks[2].data());
    data.btn = (uint8_t)std::atoi(chunks[3].data());

    return true;
}

bool write_keeloq_file(const fs::path& path, const KeeloqData& data) {
    delete_file(path);

    File file{};

    auto result = file.open(path, false, true);

    if (result) {
        return false;
    }

    std::string formatted = data.mf_name + ";" +
                            to_string_hex(data.serial) + ";" +
                            to_string_dec_uint(data.counter) + ";" +
                            to_string_dec_uint(data.btn);

    file.write_line(formatted);
    file.close();

    return true;
}
