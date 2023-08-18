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

#include "doctest.h"
#include "string_format.hpp"

/* TODO: Tests for all string_format functions. */

TEST_CASE("to_string_dec_int returns correct value.") {
    CHECK_EQ(to_string_dec_int(0), "0");
    CHECK_EQ(to_string_dec_int(1), "1");
    CHECK_EQ(to_string_dec_int(-1), "-1");
    CHECK_EQ(to_string_dec_int(1'000'000), "1000000");
    CHECK_EQ(to_string_dec_int(-1'000'000), "-1000000");
    CHECK_EQ(to_string_dec_int(1'234'567'890), "1234567890");
    CHECK_EQ(to_string_dec_int(-1'234'567'890), "-1234567890");
    CHECK_EQ(to_string_dec_int(1'234'567'891), "1234567891");
    CHECK_EQ(to_string_dec_int(-1'234'567'891), "-1234567891");
    CHECK_EQ(to_string_dec_int(9'876'543'210), "9876543210");
    CHECK_EQ(to_string_dec_int(-9'876'543'210), "-9876543210");
}

TEST_CASE("to_string_dec_uint returns correct value.") {
    CHECK_EQ(to_string_dec_uint(0), "0");
    CHECK_EQ(to_string_dec_uint(1), "1");
    CHECK_EQ(to_string_dec_uint(1'000'000), "1000000");
    CHECK_EQ(to_string_dec_uint(1'234'567'890), "1234567890");
    CHECK_EQ(to_string_dec_uint(1'234'567'891), "1234567891");
    CHECK_EQ(to_string_dec_uint(9'876'543'210), "9876543210");
}

TEST_CASE("to_string_freq returns correct value.") {
    CHECK_EQ(to_string_freq(0), "         0");
    CHECK_EQ(to_string_freq(1), "         1");
    CHECK_EQ(to_string_freq(1'000'000), "   1000000");
    CHECK_EQ(to_string_freq(1'234'567'890), "1234567890");
    CHECK_EQ(to_string_freq(1'234'567'891), "1234567891");
}

TEST_CASE("to_string_short_freq returns correct value.") {
    CHECK_EQ(to_string_short_freq(0), "   0.0000");
    CHECK_EQ(to_string_short_freq(49), "   0.0000");
    CHECK_EQ(to_string_short_freq(50), "   0.0001");
    CHECK_EQ(to_string_short_freq(1'000'049), "   1.0000");
    CHECK_EQ(to_string_short_freq(1'000'050), "   1.0001");
    CHECK_EQ(to_string_short_freq(1'234'567'850), "1234.5679");
    CHECK_EQ(to_string_short_freq(1'234'567'849), "1234.5678");
}

TEST_CASE("to_string_rounded_freq returns correct value.") {
    CHECK_EQ(to_string_rounded_freq(0, 0), "0");
    CHECK_EQ(to_string_rounded_freq(0, 2), "0.00");
    CHECK_EQ(to_string_rounded_freq(49, 4), "0.0000");
    CHECK_EQ(to_string_rounded_freq(50, 4), "0.0001");
    CHECK_EQ(to_string_rounded_freq(23'456, 4), "0.0235");
    CHECK_EQ(to_string_rounded_freq(1'000'000, 4), "1.0000");
    CHECK_EQ(to_string_rounded_freq(1'000'567'849, 4), "1000.5678");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 0), "1234");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 1), "1234.6");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 2), "1234.57");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 3), "1234.568");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 4), "1234.5679");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 5), "1234.56789");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 6), "1234.567891");
    CHECK_EQ(to_string_rounded_freq(1'234'567'891, 9), "1234.567891");
}

TEST_CASE("trim removes whitespace.") {
    CHECK(trim("  foo\n") == "foo");
}

TEST_CASE("trim returns empty for only whitespace.") {
    CHECK(trim("  \n").empty());
}

TEST_CASE("trim empty returns empty.") {
    CHECK(trim("").empty());
}
