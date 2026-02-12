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

#include "keeloq_keystore.hpp"

KeeloqKeystore::KeeloqKeystore() {
    ensure_directory(keeloq_keys_dir);

    File keeloq_keys_file{};

    if (keeloq_keys_file.open(keeloq_keys_dir / "MFCODES")) {
        return;
    }

    FileLineReader reader{keeloq_keys_file};

    for (const std::string& line : reader) {
        auto cols = split_string(line, ';');

        if (cols.size() != 3) {
            return;
        }

        KeeloqKey key{
            std::string{cols[0]},
            std::strtoull(cols[1].data(), NULL, 16),
            (uint8_t)std::atoi(cols[2].data())};

        keys.push_back(key);
    }
}

const std::vector<KeeloqKey>& KeeloqKeystore::get_keys() {
    return keys;
}
