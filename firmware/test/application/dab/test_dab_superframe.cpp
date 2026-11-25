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

#include "dab/dab_superframe.hpp"
#include "dab/dab_crc.hpp"
#include "dab/dab_reed_solomon.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {

constexpr size_t kFrameBytes = 96;
constexpr size_t kSuperframeBytes = kFrameBytes * 5;               // 480 raw bytes
constexpr size_t kWords = kSuperframeBytes / dab::kRsBlockLength;  // 4 code words
constexpr size_t kContentBytes = kWords * dab::kRsDataLength;      // 440 usable

/* Interleaves and Reed-Solomon encodes content into a transmittable
 * superframe, mirroring what rs_correct_superframe undoes. */
std::vector<uint8_t> rs_encode_superframe(const std::vector<uint8_t>& content) {
    dab::ReedSolomon rs;
    std::vector<uint8_t> raw(kSuperframeBytes, 0);
    uint8_t word[dab::kRsBlockLength];

    for (size_t i = 0; i < kWords; i++) {
        for (size_t j = 0; j < dab::kRsDataLength; j++)
            word[j] = content[i + j * kWords];
        rs.encode(word);
        for (size_t j = 0; j < dab::kRsBlockLength; j++)
            raw[i + j * kWords] = word[j];
    }
    return raw;
}

/* Builds a superframe with `count` audio units evenly spread, each carrying a
 * valid CRC, and a matching firecode. */
std::vector<uint8_t> build_superframe(uint8_t dac_rate, uint8_t sbr, uint8_t channel_mode, uint8_t ps) {
    std::vector<uint8_t> sf(kContentBytes, 0);

    const uint8_t count = dab::audio_units_for(dac_rate, sbr);
    sf[2] = static_cast<uint8_t>((dac_rate << 6) | (sbr << 5) | (channel_mode << 4) | (ps << 3));

    const size_t address_bytes = ((count - 1) * 12 + 7) / 8;
    const size_t header_bytes = 3 + address_bytes;

    const size_t usable = kContentBytes - header_bytes;
    const size_t per_unit = usable / count;

    std::vector<size_t> starts(count);
    starts[0] = header_bytes;
    for (size_t i = 1; i < count; ++i)
        starts[i] = header_bytes + i * per_unit;

    for (size_t i = 1; i < count; ++i) {
        const size_t bit_offset = (i - 1) * 12;
        const size_t byte_index = 3 + (bit_offset / 8);
        const uint16_t v = static_cast<uint16_t>(starts[i]);
        if ((bit_offset % 8) == 0) {
            sf[byte_index] = static_cast<uint8_t>(v >> 4);
            sf[byte_index + 1] = static_cast<uint8_t>((sf[byte_index + 1] & 0x0F) | ((v & 0x0F) << 4));
        } else {
            sf[byte_index] = static_cast<uint8_t>((sf[byte_index] & 0xF0) | ((v >> 8) & 0x0F));
            sf[byte_index + 1] = static_cast<uint8_t>(v & 0xFF);
        }
    }

    for (size_t i = 0; i < count; ++i) {
        const size_t end = (i + 1 < count) ? starts[i + 1] : kContentBytes;
        const size_t len = end - starts[i];
        for (size_t k = 0; k + 2 < len; ++k)
            sf[starts[i] + k] = static_cast<uint8_t>(0x40 + i * 5 + k);
        dab::audio_unit_crc_stamp(&sf[starts[i]], len);
    }

    dab::firecode_stamp(sf.data());
    return rs_encode_superframe(sf);
}

void push_all(dab::SuperframeAssembler& a, const std::vector<uint8_t>& sf, bool* ready_out) {
    for (size_t f = 0; f < 5; ++f)
        ready_out[f] = a.push_logical_frame(&sf[f * kFrameBytes]);
}

}  // namespace

TEST_CASE("audio unit counts follow the rate and sbr flags") {
    CHECK(dab::audio_units_for(0, 1) == 2);
    CHECK(dab::audio_units_for(1, 1) == 3);
    CHECK(dab::audio_units_for(0, 0) == 4);
    CHECK(dab::audio_units_for(1, 0) == 6);
}

TEST_CASE("configure rejects impossible frame sizes") {
    dab::SuperframeAssembler a;
    CHECK_FALSE(a.configure(0));
    CHECK_FALSE(a.configure(dab::kMaxLogicalFrameBytes + 1));
    CHECK(a.configure(kFrameBytes));
}

TEST_CASE("superframe becomes ready only on the fifth logical frame") {
    auto sf = build_superframe(1, 1, 1, 0);
    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));

    bool ready[5];
    push_all(a, sf, ready);

    CHECK_FALSE(ready[0]);
    CHECK_FALSE(ready[1]);
    CHECK_FALSE(ready[2]);
    CHECK_FALSE(ready[3]);
    CHECK(ready[4]);
}

TEST_CASE("header fields are parsed") {
    auto sf = build_superframe(1, 1, 1, 0);
    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));

    bool ready[5];
    push_all(a, sf, ready);
    REQUIRE(ready[4]);

    const auto& h = a.header();
    CHECK(h.dac_rate == 1);
    CHECK(h.sbr_flag == 1);
    CHECK(h.aac_channel_mode == 1);
    CHECK(h.ps_flag == 0);
    CHECK(h.audio_unit_count == 3);
    CHECK(h.core_sample_rate() == 24000);
    CHECK(h.output_sample_rate() == 48000);
}

TEST_CASE("audio units are extracted with valid CRCs") {
    auto sf = build_superframe(0, 0, 1, 0);  // 4 units
    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));

    bool ready[5];
    push_all(a, sf, ready);
    REQUIRE(ready[4]);

    REQUIRE(a.audio_unit_count() == 4);
    for (size_t i = 0; i < a.audio_unit_count(); ++i) {
        CHECK(a.audio_units()[i].length > 0);
        CHECK(a.audio_units()[i].crc_ok);
    }
}

TEST_CASE("Reed-Solomon repairs damage that would otherwise break the frame") {
    auto sf = build_superframe(0, 0, 1, 0);
    // One byte per code word is well within RS(120,110)'s five-byte capability.
    for (size_t i = 0; i < kWords; ++i)
        sf[i + 7 * kWords] ^= 0xFF;

    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));
    bool ready[5];
    push_all(a, sf, ready);

    REQUIRE(ready[4]);
    for (size_t i = 0; i < a.audio_unit_count(); ++i)
        CHECK(a.audio_units()[i].crc_ok);
}

TEST_CASE("damage beyond the code's capability is rejected, not parsed") {
    auto sf = build_superframe(0, 0, 1, 0);
    // Six errors in one code word exceed t = 5.
    for (size_t j = 0; j < 6; ++j)
        sf[0 + j * kWords] ^= 0xA5;

    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));
    bool ready[5];
    push_all(a, sf, ready);
    CHECK_FALSE(ready[4]);
}

TEST_CASE("a superframe that is not a whole number of code words is refused") {
    dab::SuperframeAssembler a;
    // 5 * 100 = 500 is not a multiple of 120.
    CHECK_FALSE(a.configure(100));
}

TEST_CASE("rs_correct_superframe round trips") {
    std::vector<uint8_t> content(kContentBytes);
    for (size_t i = 0; i < content.size(); ++i)
        content[i] = static_cast<uint8_t>(i * 7 + 1);

    auto raw = rs_encode_superframe(content);
    std::vector<uint8_t> out(kContentBytes, 0);

    REQUIRE(dab::rs_correct_superframe(raw.data(), raw.size(), out.data(), out.size()) == kContentBytes);
    for (size_t i = 0; i < kContentBytes; ++i)
        REQUIRE(out[i] == content[i]);
}

TEST_CASE("rs_correct_superframe rejects bad arguments") {
    std::vector<uint8_t> raw(kSuperframeBytes, 0);
    std::vector<uint8_t> out(kContentBytes, 0);

    CHECK(dab::rs_correct_superframe(nullptr, raw.size(), out.data(), out.size()) == 0);
    CHECK(dab::rs_correct_superframe(raw.data(), raw.size(), nullptr, out.size()) == 0);
    CHECK(dab::rs_correct_superframe(raw.data(), 100, out.data(), out.size()) == 0);
    CHECK(dab::rs_correct_superframe(raw.data(), raw.size(), out.data(), 10) == 0);
}

TEST_CASE("a bad firecode is not accepted as a superframe") {
    // Corrupt enough code words that RS cannot restore the header.
    auto sf = build_superframe(1, 1, 1, 0);
    for (size_t i = 0; i < kWords; ++i)
        for (size_t j = 0; j < 6; ++j)
            sf[i + j * kWords] ^= 0x5A;

    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));
    bool ready[5];
    push_all(a, sf, ready);
    CHECK_FALSE(ready[4]);
    CHECK_FALSE(a.synchronised());
}

TEST_CASE("resynchronises when started mid superframe") {
    auto sf = build_superframe(1, 1, 1, 0);
    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));

    // Start two frames late, then feed a continuous stream of superframes.
    bool locked = false;
    for (size_t f = 2; f < 5; ++f)
        a.push_logical_frame(&sf[f * kFrameBytes]);

    for (int repeat = 0; repeat < 4 && !locked; ++repeat) {
        for (size_t f = 0; f < 5; ++f) {
            if (a.push_logical_frame(&sf[f * kFrameBytes]))
                locked = true;
        }
    }

    CHECK(locked);
    CHECK(a.synchronised());
}

TEST_CASE("reset drops synchronisation") {
    auto sf = build_superframe(1, 1, 1, 0);
    dab::SuperframeAssembler a;
    REQUIRE(a.configure(kFrameBytes));
    bool ready[5];
    push_all(a, sf, ready);
    REQUIRE(a.synchronised());

    a.reset();
    CHECK_FALSE(a.synchronised());
}

TEST_CASE("push rejects null and unconfigured use") {
    dab::SuperframeAssembler a;
    std::vector<uint8_t> buf(kFrameBytes, 0);
    CHECK_FALSE(a.push_logical_frame(buf.data()));
    REQUIRE(a.configure(kFrameBytes));
    CHECK_FALSE(a.push_logical_frame(nullptr));
}

TEST_CASE("audio unit crc round trip") {
    uint8_t unit[32]{};
    for (size_t i = 0; i < 30; ++i)
        unit[i] = static_cast<uint8_t>(i * 3);
    dab::audio_unit_crc_stamp(unit, sizeof(unit));
    CHECK(dab::audio_unit_crc_ok(unit, sizeof(unit)));
    unit[10] ^= 0x01;
    CHECK_FALSE(dab::audio_unit_crc_ok(unit, sizeof(unit)));
}
