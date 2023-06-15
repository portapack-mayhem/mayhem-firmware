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

#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <charconv>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>

/* Zero-allocation conversion helper. */
/* Notes:
 * - T must be an integer type.
 * - For base 16, input _must not_ contain a '0x' or '0X' prefix.
 * - For base 8 a leading 0 on the literal is allowed.
 * - Leading whitespce will cause the parse to fail.
 */

template <typename T>
std::enable_if_t<std::is_integral_v<T>, bool> parse_int(std::string_view str, T& out_val, int base = 10) {
    auto result = std::from_chars(str.data(), str.data() + str.length(), out_val, base);
    return static_cast<int>(result.ec) == 0;
}

#endif /*__CONVERT_H__*/