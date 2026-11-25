/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "dab/dab_aac.hpp"

namespace {

dab::SuperframeHeader header(uint8_t dac_rate, uint8_t sbr, uint8_t channel_mode, uint8_t ps) {
    dab::SuperframeHeader h{};
    h.dac_rate = dac_rate;
    h.sbr_flag = sbr;
    h.aac_channel_mode = channel_mode;
    h.ps_flag = ps;
    h.audio_unit_count = dab::audio_units_for(dac_rate, sbr);
    return h;
}

/* Reads the config back field by field, so the test checks the packing rather
 * than restating the same expression. */
struct Unpacked {
    uint8_t object_type;
    uint8_t frequency_index;
    uint8_t channels;
    uint8_t frame_length_flag;
};

Unpacked unpack(const dab::AacConfig& c) {
    const uint16_t v = static_cast<uint16_t>((c.bytes[0] << 8) | c.bytes[1]);
    return Unpacked{
        static_cast<uint8_t>((v >> 11) & 0x1F),
        static_cast<uint8_t>((v >> 7) & 0x0F),
        static_cast<uint8_t>((v >> 3) & 0x0F),
        static_cast<uint8_t>((v >> 2) & 0x01)};
}

}  // namespace

TEST_CASE("sampling frequency indices match the MPEG-4 table") {
    CHECK(dab::aac_sampling_frequency_index(48000) == 3);
    CHECK(dab::aac_sampling_frequency_index(32000) == 5);
    CHECK(dab::aac_sampling_frequency_index(24000) == 6);
    CHECK(dab::aac_sampling_frequency_index(16000) == 8);
    // Anything outside the table must report the escape value, not index 0.
    CHECK(dab::aac_sampling_frequency_index(37000) == 0x0F);
    CHECK(dab::aac_sampling_frequency_index(0) == 0x0F);
}

TEST_CASE("the config always selects the 960 transform") {
    // This is the one that makes DAB+ different from ordinary AAC: a decoder
    // configured for 1024 produces nothing usable.
    for (uint8_t dac = 0; dac < 2; dac++) {
        for (uint8_t sbr = 0; sbr < 2; sbr++) {
            const auto c = dab::aac_config_from_superframe(header(dac, sbr, 1, 0));
            REQUIRE(c.valid);
            CHECK(unpack(c).frame_length_flag == 1);
        }
    }
}

TEST_CASE("48 kHz output with SBR configures a 24 kHz core") {
    const auto c = dab::aac_config_from_superframe(header(1, 1, 1, 0));
    REQUIRE(c.valid);

    CHECK(c.output_rate == 48000);
    CHECK(c.core_rate == 24000);
    CHECK(c.sbr);
    CHECK(unpack(c).frequency_index == 6);  // 24000
    CHECK(unpack(c).object_type == dab::kAacObjectTypeLc);
    CHECK(unpack(c).channels == 2);
}

TEST_CASE("32 kHz output with SBR configures a 16 kHz core") {
    const auto c = dab::aac_config_from_superframe(header(0, 1, 1, 0));
    REQUIRE(c.valid);
    CHECK(c.output_rate == 32000);
    CHECK(c.core_rate == 16000);
    CHECK(unpack(c).frequency_index == 8);  // 16000
}

TEST_CASE("without SBR the core runs at the output rate") {
    const auto c48 = dab::aac_config_from_superframe(header(1, 0, 1, 0));
    REQUIRE(c48.valid);
    CHECK(c48.core_rate == 48000);
    CHECK(c48.output_rate == 48000);
    CHECK_FALSE(c48.sbr);
    CHECK(unpack(c48).frequency_index == 3);

    const auto c32 = dab::aac_config_from_superframe(header(0, 0, 1, 0));
    REQUIRE(c32.valid);
    CHECK(c32.core_rate == 32000);
    CHECK(unpack(c32).frequency_index == 5);
}

TEST_CASE("mono and stereo channel configurations") {
    CHECK(unpack(dab::aac_config_from_superframe(header(1, 1, 0, 0))).channels == 1);
    CHECK(unpack(dab::aac_config_from_superframe(header(1, 1, 1, 0))).channels == 2);
}

TEST_CASE("parametric stereo is reported so the caller knows it is mono") {
    // Helix and FAAD2 without PS decode a PS stream's core, which is mono.
    // Surfacing the flag lets the receiver say so rather than claim stereo.
    const auto c = dab::aac_config_from_superframe(header(1, 1, 0, 1));
    REQUIRE(c.valid);
    CHECK(c.parametric_stereo);
    CHECK(c.channels == 1);
}

TEST_CASE("a header describing no audio units yields no config") {
    dab::SuperframeHeader h{};
    h.audio_unit_count = 0;
    CHECK_FALSE(dab::aac_config_from_superframe(h).valid);
}

TEST_CASE("the config is exactly two bytes") {
    const auto c = dab::aac_config_from_superframe(header(1, 1, 1, 0));
    REQUIRE(c.valid);
    CHECK(c.length == 2);
}
