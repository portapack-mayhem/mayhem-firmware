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
#include "utility.hpp"

TEST_SUITE_BEGIN("Flags operators");

enum class Flags : uint8_t {
    A = 0x1,
    B = 0x2,
    C = 0x4,
};

ENABLE_FLAGS_OPERATORS(Flags);

TEST_CASE("operator| should combine flags.") {
    constexpr Flags f = Flags::A | Flags::C;
    static_assert(static_cast<int>(f) == 5);
}

TEST_CASE("When flag set, flags_enabled should be true.") {
    constexpr Flags f = Flags::A;
    static_assert(flags_enabled(f, Flags::A));
}

TEST_CASE("When flag not set, flags_enabled should be false.") {
    constexpr Flags f = Flags::B;
    static_assert(flags_enabled(f, Flags::A) == false);
}

TEST_SUITE_END();

TEST_CASE("Stash should save and restore value.") {
    int val = 10;

    {
        Stash s{val};
        val = 20;
        CHECK_EQ(val, 20);
    }

    CHECK_EQ(val, 10);
}

TEST_CASE("ms_duration should return duration.") {
    auto sample_rate = 48'000;
    auto bytes_per_sample = 2;  // 16 bit.
    auto seconds = 5;
    auto size = seconds * sample_rate * bytes_per_sample;
    CHECK_EQ(ms_duration(size, sample_rate, bytes_per_sample), 5 * 1000);
}

TEST_CASE("ms_duration not fault when passed zero.") {
    CHECK_EQ(ms_duration(0, 0, 0), 0);
}