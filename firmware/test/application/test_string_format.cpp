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

TEST_CASE("to_string_dec_uint64 returns correct value.") {
    CHECK_EQ(to_string_dec_uint64(0), "0");
    CHECK_EQ(to_string_dec_uint64(1), "1");
    CHECK_EQ(to_string_dec_uint64(1'000'000), "1000000");
    CHECK_EQ(to_string_dec_uint64(1'234'567'890), "1234567890");
    CHECK_EQ(to_string_dec_uint64(1'234'567'891), "1234567891");
}

/*TEST_CASE("to_string_freq returns correct value.") {
    CHECK_EQ(to_string_freq(0), "0");
    CHECK_EQ(to_string_freq(1), "1");
    CHECK_EQ(to_string_freq(1'000'000), "1000000");
    CHECK_EQ(to_string_freq(1'234'567'890), "1234567890");
    CHECK_EQ(to_string_freq(1'234'567'891), "1234567891");
}*/

TEST_CASE("trim removes whitespace.") {
    CHECK(trim("  foo\n") == "foo");
}

TEST_CASE("trim returns empty for only whitespace.") {
    CHECK(trim("  \n").empty());
}
