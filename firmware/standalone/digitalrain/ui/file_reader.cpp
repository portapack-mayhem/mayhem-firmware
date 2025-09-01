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

#include "file_reader.hpp"
#include <string_view>
#include <vector>

/* Splits the string on the specified char and returns
 * a vector of string_views. NB: the lifetime of the
 * string to split must be maintained while the views
 * are used or they will dangle. */
std::vector<std::string_view> split_string(std::string_view str, char c) {
    std::vector<std::string_view> cols;
    size_t start = 0;

    while (start < str.length()) {
        auto it = str.find(c, start);

        if (it == str.npos)
            break;

        cols.emplace_back(&str[start], it - start);
        start = it + 1;
    }

    if (start <= str.length() && !str.empty())
        cols.emplace_back(&str[start], str.length() - start);

    return cols;
}
