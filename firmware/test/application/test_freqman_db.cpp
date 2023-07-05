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
#include "freqman_db.hpp"

TEST_SUITE_BEGIN("Freqman Parsing");

TEST_CASE("It can parse basic single freq entry.") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.", e));

    CHECK_EQ(e.frequency_a, 123'000'000);
    CHECK_EQ(e.description, "This is the description.");
    CHECK_EQ(e.type, freqman_type::Single);
}

TEST_CASE("It can parse basic range freq entry.") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "a=123000000,b=423000000,d=This is the description.", e));

    CHECK_EQ(e.frequency_a, 123'000'000);
    CHECK_EQ(e.frequency_b, 423'000'000);
    CHECK_EQ(e.description, "This is the description.");
    CHECK_EQ(e.type, freqman_type::Range);
}

TEST_CASE("It can parse basic ham radio freq entry.") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "r=123000000,t=423000000,d=This is the description.", e));

    CHECK_EQ(e.frequency_a, 123'000'000);
    CHECK_EQ(e.frequency_b, 423'000'000);
    CHECK_EQ(e.description, "This is the description.");
    CHECK_EQ(e.type, freqman_type::HamRadio);
}

TEST_CASE("It can parse modulation") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=AM", e));
    CHECK_EQ(e.modulation, 0);
    
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=NFM", e));
    CHECK_EQ(e.modulation, 1);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=WFM", e));
    CHECK_EQ(e.modulation, 2);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=SPEC", e));
    CHECK_EQ(e.modulation, 3);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=FOO", e));
    CHECK_EQ(e.modulation, freqman_invalid_index);
}

TEST_CASE("It can parse frequency step") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,s=0.1kHz", e));
    CHECK_EQ(e.step, 0);
    
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,s=50kHz", e));
    CHECK_EQ(e.step, 11);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,s=FOO", e));
    CHECK_EQ(e.step, freqman_invalid_index);
}

#if 0  // New tables for a future PR.
TEST_CASE("It can parse modulation") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=AM", e));
    CHECK_EQ(e.modulation, freqman_modulation::AM);
    
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=NFM", e));
    CHECK_EQ(e.modulation, freqman_modulation::NFM);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=WFM", e));
    CHECK_EQ(e.modulation, freqman_modulation::WFM);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=SPEC", e));
    CHECK_EQ(e.modulation, freqman_modulation::SPEC);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,m=FOO", e));
    CHECK_EQ(e.modulation, freqman_modulation::Unknown);
}

TEST_CASE("It can parse frequency step") {
    freqman_entry e;
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,s=0.1kHz", e));
    CHECK_EQ(e.step, freqman_step::_100Hz);
    
    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,s=50kHz", e));
    CHECK_EQ(e.step, freqman_step::_50kHz);

    REQUIRE(
        parse_freqman_entry(
            "f=123000000,d=This is the description.,s=FOO", e));
    CHECK_EQ(e.step, freqman_step::Unknown);
}
#endif


TEST_SUITE_END();
