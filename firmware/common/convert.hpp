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

#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

constexpr size_t max_parse_int_length = 64;

/* This undefined type is used to see the size of the
 * unhandled type and to create a compilation error. */
template <size_t N>
struct UnhandledSize;

/* Returns true when errno is ERANGE. */
inline bool range_error() {
    return errno == ERANGE;
}

/* Assigns 'value' to 'out_val' and returns true if 'value' is
 * in bounds for type TOut. */
template <typename TVal, typename TOut>
bool checked_assign(TVal value, TOut& out_val) {
    // No chance for overflow, just return.
    if constexpr (sizeof(TVal) <= sizeof(TOut)) {
        out_val = value;
        return true;
    }
    out_val = static_cast<TOut>(value);
    return value >= static_cast<TVal>(std::numeric_limits<TOut>::min()) &&
           value <= static_cast<TVal>(std::numeric_limits<TOut>::max());
}

/* Zero-allocation conversion helper. 'str' must be smaller than 'max_parse_int_length'. */
template <typename T>
std::enable_if_t<std::is_integral_v<T>, bool> parse_int(std::string_view str, T& out_val, int base = 10) {
    // Always initialize the output.
    out_val = {};

    if (str.size() > max_parse_int_length)
        return false;

    // Copy onto the stack and null terminate.
    char zstr[max_parse_int_length + 1];
    std::memcpy(zstr, str.data(), str.size());
    zstr[str.size()] = '\0';

    // A little C++ type magic to select the correct conversion function.
    if constexpr (sizeof(T) == sizeof(long long)) {
        if constexpr (std::is_unsigned_v<T>)
            return checked_assign(strtoull(zstr, nullptr, base), out_val) && !range_error();
        else
            return checked_assign(strtoll(zstr, nullptr, base), out_val) && !range_error();

    } else if constexpr (sizeof(T) <= sizeof(long)) {
        if constexpr (std::is_unsigned_v<T>)
            return checked_assign(strtoul(zstr, nullptr, base), out_val) && !range_error();
        else
            return checked_assign(strtol(zstr, nullptr, base), out_val) && !range_error();
    } else {
        UnhandledSize<sizeof(T) * 8> unhandled_case;
        return false;
    }

    return true;
}

#endif /*__CONVERT_H__*/