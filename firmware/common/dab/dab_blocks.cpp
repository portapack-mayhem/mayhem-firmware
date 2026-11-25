/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "dab_blocks.hpp"

#include <cstring>

namespace dab {

namespace {

/* Ascending frequency, so a sweep walks the table in order. */
constexpr Block band_iii[] = {
    {"5A", 174928000},
    {"5B", 176640000},
    {"5C", 178352000},
    {"5D", 180064000},
    {"6A", 181936000},
    {"6B", 183648000},
    {"6C", 185360000},
    {"6D", 187072000},
    {"7A", 188928000},
    {"7B", 190640000},
    {"7C", 192352000},
    {"7D", 194064000},
    {"8A", 195936000},
    {"8B", 197648000},
    {"8C", 199360000},
    {"8D", 201072000},
    {"9A", 202928000},
    {"9B", 204640000},
    {"9C", 206352000},
    {"9D", 208064000},
    {"10A", 209936000},
    {"10N", 210096000},
    {"10B", 211648000},
    {"10C", 213360000},
    {"10D", 215072000},
    {"11A", 216928000},
    {"11N", 217088000},
    {"11B", 218640000},
    {"11C", 220352000},
    {"11D", 222064000},
    {"12A", 223936000},
    {"12N", 224096000},
    {"12B", 225648000},
    {"12C", 227360000},
    {"12D", 229072000},
    {"13A", 230784000},
    {"13B", 232496000},
    {"13C", 234208000},
    {"13D", 235776000},
    {"13E", 237488000},
    {"13F", 239200000},
};

constexpr Block l_band[] = {
    {"LA", 1452960000},
    {"LB", 1454672000},
    {"LC", 1456384000},
    {"LD", 1458096000},
    {"LE", 1459808000},
    {"LF", 1461520000},
    {"LG", 1463232000},
    {"LH", 1464944000},
    {"LI", 1466656000},
    {"LJ", 1468368000},
    {"LK", 1470080000},
    {"LL", 1471792000},
    {"LM", 1473504000},
    {"LN", 1475216000},
    {"LO", 1476928000},
    {"LP", 1478640000},
    {"LQ", 1480352000},
    {"LR", 1482064000},
    {"LS", 1483776000},
    {"LT", 1485488000},
    {"LU", 1487200000},
    {"LV", 1488912000},
    {"LW", 1490624000},
};

const Block* find_in(const Block* table, size_t count, const char* name) {
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(table[i].name, name) == 0)
            return &table[i];
    }
    return nullptr;
}

}  // namespace

BlockTable band_iii_blocks() {
    return {band_iii, sizeof(band_iii) / sizeof(band_iii[0])};
}

BlockTable l_band_blocks() {
    return {l_band, sizeof(l_band) / sizeof(l_band[0])};
}

const Block* find_block(const char* name) {
    if (name == nullptr || name[0] == '\0')
        return nullptr;

    const auto biii = band_iii_blocks();
    if (const auto* found = find_in(biii.items, biii.count, name))
        return found;

    const auto lb = l_band_blocks();
    return find_in(lb.items, lb.count, name);
}

} /* namespace dab */
