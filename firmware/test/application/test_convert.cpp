/*
 * Copyright (C) 2023
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
#include "convert.hpp"
#include <string>
#include <string_view>

TEST_SUITE_BEGIN("parse_int");

TEST_CASE("It should convert literals.") {
    int val = 0;
    REQUIRE(parse_int("12345", val));
    CHECK_EQ(val, 12345);
}

TEST_CASE("It should convert strings.") {
    int val = 0;
    std::string s = "12345";
    REQUIRE(parse_int(s, val));
    CHECK_EQ(val, 12345);
}

TEST_CASE("It should convert string_views.") {
    int val = 0;
    std::string_view s{"12345"};
    REQUIRE(parse_int(s, val));
    CHECK_EQ(val, 12345);
}

// 'from_chars' supported this, but 'strtol' just returns 0.
/*TEST_CASE("It should return false for invalid input") {
    int val = 0;
    REQUIRE_FALSE(parse_int("QWERTY", val));
}*/

TEST_CASE("It should return false for overflow input") {
    uint8_t val = 0;
    REQUIRE_FALSE(parse_int("500", val));
}

TEST_CASE("It should convert base 16.") {
    int val = 0;
    REQUIRE(parse_int("0x30", val, 16));
    CHECK_EQ(val, 0x30);
}

TEST_CASE("It should convert base 8.") {
    int val = 0;
    REQUIRE(parse_int("020", val, 8));
    CHECK_EQ(val, 020);
}

TEST_CASE("It should convert as much of the string as it can.") {
    int val = 0;
    REQUIRE(parse_int("12345foobar", val));
    CHECK_EQ(val, 12345);
}

TEST_CASE("It should convert 64-bit.") {
    int64_t val = 0;
    REQUIRE(parse_int("123456789", val));
    CHECK_EQ(val, 123456789);
}

TEST_CASE("It should convert 32-bit.") {
    int32_t val = 0;
    REQUIRE(parse_int("123456", val));
    CHECK_EQ(val, 123456);
}

TEST_CASE("It should convert 16-bit.") {
    int16_t val = 0;
    REQUIRE(parse_int("12345", val));
    CHECK_EQ(val, 12345);
}

TEST_CASE("It should convert 8-bit.") {
    int8_t val = 0;
    REQUIRE(parse_int("123", val));
    CHECK_EQ(val, 123);
}

TEST_CASE("It should convert negative.") {
    int8_t val = 0;
    REQUIRE(parse_int("-64", val));
    CHECK_EQ(val, -64);
}

TEST_SUITE_END();