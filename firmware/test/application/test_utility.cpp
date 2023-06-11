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

TEST_SUITE_BEGIN("optional");

enum class Flags : uint8_t {
    A = 0x1,
    B = 0x2,
    C = 0x4,
};

TEST_CASE("When flag set, flags_enabled should be true.") {
    Flags f = Flags::A;
    CHECK(flags_enabled(f, Flags::A));
}

TEST_CASE("When flag not set, flags_enabled should be false.") {
    Flags f = Flags::B;
    CHECK(flags_enabled(f, Flags::A) == false);
}

TEST_SUITE_END();