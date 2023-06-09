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
#include "optional.hpp"

TEST_SUITE_BEGIN("optional");

TEST_CASE("Default instance should not be valid.") {
    Optional<int> o;
    REQUIRE_FALSE(o.is_valid());
}

TEST_CASE("Instance with value should be valid.") {
    Optional<int> o{1};
    REQUIRE(o.is_valid());
}

TEST_CASE("Instance with value should be return true.") {
    Optional<int> o{1};
    REQUIRE((bool)o);
}

TEST_CASE("value() should return value.") {
    Optional<int> o{1};
    REQUIRE(o.value() == 1);
}

TEST_CASE("operator* should return value.") {
    Optional<int> o{1};
    REQUIRE(*o == 1);
}

TEST_CASE("operator-> should return value members.") {
    struct S {
        int i;
    };

    Optional<S> o{S{1}};
    REQUIRE(o->i == 1);
}

TEST_SUITE_END();