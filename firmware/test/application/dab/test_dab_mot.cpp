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

#include "dab/dab_mot.hpp"

#include <cstring>
#include <vector>

namespace {

/* Builds an MSC data group: segmented, with a transport id, no CRC. */
std::vector<uint8_t> group(uint8_t type, uint16_t transport_id, uint16_t seg_num, bool last, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> g;
    g.push_back(static_cast<uint8_t>(0x30 | type));  // segment + user access flags
    g.push_back(0x00);
    g.push_back(static_cast<uint8_t>((last ? 0x80 : 0x00) | ((seg_num >> 8) & 0x7F)));
    g.push_back(static_cast<uint8_t>(seg_num & 0xFF));
    g.push_back(0x12);  // transport id flag set, length indicator 2
    g.push_back(static_cast<uint8_t>(transport_id >> 8));
    g.push_back(static_cast<uint8_t>(transport_id & 0xFF));
    for (uint8_t b : payload)
        g.push_back(b);
    return g;
}

std::vector<uint8_t> mot_header(uint32_t body_size, uint8_t content_type, uint8_t subtype) {
    std::vector<uint8_t> h(7, 0);
    h[0] = static_cast<uint8_t>((body_size >> 20) & 0xFF);
    h[1] = static_cast<uint8_t>((body_size >> 12) & 0xFF);
    h[2] = static_cast<uint8_t>((body_size >> 4) & 0xFF);
    h[3] = static_cast<uint8_t>((body_size & 0x0F) << 4);
    // ContentType is bits 41..46, ContentSubType bits 47..55.
    h[5] = static_cast<uint8_t>(((content_type & 0x3F) << 1) | ((subtype >> 8) & 0x01));
    h[6] = static_cast<uint8_t>(subtype & 0xFF);
    return h;
}

std::vector<uint8_t> body(size_t n, uint8_t seed) {
    std::vector<uint8_t> v(n);
    for (size_t i = 0; i < n; ++i)
        v[i] = static_cast<uint8_t>(seed + i);
    return v;
}

}  // namespace

TEST_CASE("a header plus four body segments reassembles") {
    dab::MotAssembler m;

    auto s0 = body(64, 0);
    auto s1 = body(64, 64);
    auto s2 = body(64, 128);
    auto s3 = body(20, 192);

    auto h = group(3, 0x1234, 0, true, mot_header(64 * 3 + 20, dab::kMotContentTypeImage, 1));
    CHECK_FALSE(m.push_data_group(h.data(), h.size()));

    auto g0 = group(4, 0x1234, 0, false, s0);
    auto g1 = group(4, 0x1234, 1, false, s1);
    auto g2 = group(4, 0x1234, 2, false, s2);
    auto g3 = group(4, 0x1234, 3, true, s3);

    m.push_data_group(g0.data(), g0.size());
    m.push_data_group(g1.data(), g1.size());
    m.push_data_group(g2.data(), g2.size());
    CHECK(m.push_data_group(g3.data(), g3.size()));

    CHECK(m.image_size() == 64 * 3 + 20);
    CHECK(m.content_type() == dab::kMotContentTypeImage);
    CHECK(m.content_subtype() == 1);
    CHECK(m.take_new_image());

    // Verify the reassembled bytes.
    std::vector<uint8_t> expected;
    expected.insert(expected.end(), s0.begin(), s0.end());
    expected.insert(expected.end(), s1.begin(), s1.end());
    expected.insert(expected.end(), s2.begin(), s2.end());
    expected.insert(expected.end(), s3.begin(), s3.end());
    CHECK(std::memcmp(m.image(), expected.data(), expected.size()) == 0);
}

TEST_CASE("segments arriving out of order reassemble correctly") {
    dab::MotAssembler m;
    auto s0 = body(32, 1);
    auto s1 = body(32, 33);
    auto s2 = body(10, 65);

    auto g0 = group(4, 0x99, 0, false, s0);
    auto g1 = group(4, 0x99, 1, false, s1);
    auto g2 = group(4, 0x99, 2, true, s2);

    m.push_data_group(g1.data(), g1.size());
    m.push_data_group(g2.data(), g2.size());
    CHECK(m.push_data_group(g0.data(), g0.size()));
    CHECK(m.image_size() == 74);

    std::vector<uint8_t> expected;
    expected.insert(expected.end(), s0.begin(), s0.end());
    expected.insert(expected.end(), s1.begin(), s1.end());
    expected.insert(expected.end(), s2.begin(), s2.end());
    CHECK(std::memcmp(m.image(), expected.data(), expected.size()) == 0);
}

TEST_CASE("two interleaved transport ids do not contaminate each other") {
    dab::MotAssembler m;

    auto a0 = body(16, 10);
    auto b0 = body(16, 200);
    auto b1 = body(8, 216);

    auto ga0 = group(4, 0x1111, 0, false, a0);
    m.push_data_group(ga0.data(), ga0.size());

    // A new transport id restarts assembly.
    auto gb0 = group(4, 0x2222, 0, false, b0);
    auto gb1 = group(4, 0x2222, 1, true, b1);
    m.push_data_group(gb0.data(), gb0.size());
    CHECK(m.push_data_group(gb1.data(), gb1.size()));

    CHECK(m.transport_id() == 0x2222);
    CHECK(m.image_size() == 24);
    CHECK(std::memcmp(m.image(), b0.data(), b0.size()) == 0);
}

TEST_CASE("an oversized object is abandoned without overrunning") {
    dab::MotAssembler m;
    auto h = group(3, 0x1234, 0, true, mot_header(200000, dab::kMotContentTypeImage, 1));
    CHECK_FALSE(m.push_data_group(h.data(), h.size()));

    // Subsequent segments for the abandoned object are refused.
    auto g0 = group(4, 0x1234, 0, true, body(64, 0));
    CHECK_FALSE(m.push_data_group(g0.data(), g0.size()));
    CHECK(m.image_size() == 0);
}

TEST_CASE("a declared size mismatch does not publish") {
    dab::MotAssembler m;
    auto h = group(3, 0x1234, 0, true, mot_header(1000, dab::kMotContentTypeImage, 1));
    m.push_data_group(h.data(), h.size());

    auto g0 = group(4, 0x1234, 0, true, body(64, 0));
    CHECK_FALSE(m.push_data_group(g0.data(), g0.size()));
}

TEST_CASE("a missing segment does not publish") {
    dab::MotAssembler m;
    auto g0 = group(4, 0x55, 0, false, body(32, 0));
    auto g2 = group(4, 0x55, 2, true, body(32, 64));
    m.push_data_group(g0.data(), g0.size());
    CHECK_FALSE(m.push_data_group(g2.data(), g2.size()));
}

TEST_CASE("a last segment arriving first is placed once the stride is known") {
    dab::MotAssembler m;
    auto s0 = body(32, 1);
    auto s1 = body(32, 33);
    auto s2 = body(10, 65);

    // Last segment first: its offset cannot be known yet.
    auto g2 = group(4, 0x77, 2, true, s2);
    CHECK_FALSE(m.push_data_group(g2.data(), g2.size()));

    auto g1 = group(4, 0x77, 1, false, s1);
    m.push_data_group(g1.data(), g1.size());

    auto g0 = group(4, 0x77, 0, false, s0);
    CHECK(m.push_data_group(g0.data(), g0.size()));

    REQUIRE(m.image_size() == 74);
    std::vector<uint8_t> expected;
    expected.insert(expected.end(), s0.begin(), s0.end());
    expected.insert(expected.end(), s1.begin(), s1.end());
    expected.insert(expected.end(), s2.begin(), s2.end());
    CHECK(std::memcmp(m.image(), expected.data(), expected.size()) == 0);
}

TEST_CASE("a single-segment object uses its own length as the stride") {
    dab::MotAssembler m;
    auto only = body(24, 7);
    auto g = group(4, 0x88, 0, true, only);
    CHECK(m.push_data_group(g.data(), g.size()));
    CHECK(m.image_size() == 24);
    CHECK(std::memcmp(m.image(), only.data(), only.size()) == 0);
}

TEST_CASE("a segment number beyond capacity is rejected") {
    dab::MotAssembler m;
    auto g = group(4, 0x77, dab::kMaxMotSegments + 5, true, body(16, 0));
    CHECK_FALSE(m.push_data_group(g.data(), g.size()));
}

TEST_CASE("truncated data groups are refused without reading out of bounds") {
    dab::MotAssembler m;
    CHECK_FALSE(m.push_data_group(nullptr, 10));

    const uint8_t tiny[1] = {0x34};
    CHECK_FALSE(m.push_data_group(tiny, 1));

    // Declares segmentation and user access but supplies neither.
    const uint8_t truncated[3] = {0x34, 0x00, 0x80};
    CHECK_FALSE(m.push_data_group(truncated, 3));
}

TEST_CASE("non MOT group types are ignored") {
    dab::MotAssembler m;
    auto g = group(1, 0x11, 0, true, body(16, 0));
    CHECK_FALSE(m.push_data_group(g.data(), g.size()));
}

TEST_CASE("reset clears the object") {
    dab::MotAssembler m;
    auto g = group(4, 0x33, 0, true, body(16, 0));
    m.push_data_group(g.data(), g.size());
    m.reset();
    CHECK(m.image_size() == 0);
    CHECK_FALSE(m.take_new_image());
}

TEST_CASE("a corrupted carousel pass is retried when the object repeats") {
    // Slideshow repeats the same transport id. A bad segment must not kill
    // reassembly of that image until some unrelated object happens along.
    dab::MotAssembler m;

    auto good0 = body(32, 1);
    auto g0 = group(4, 0x55, 0, false, good0);
    m.push_data_group(g0.data(), g0.size());

    // A middle segment of the wrong length abandons this pass.
    auto bad = body(20, 99);
    auto g1 = group(4, 0x55, 1, false, bad);
    CHECK_FALSE(m.push_data_group(g1.data(), g1.size()));

    // The carousel comes round: segment 0 of the same id starts a fresh pass.
    auto r0 = group(4, 0x55, 0, false, good0);
    m.push_data_group(r0.data(), r0.size());
    auto r1 = group(4, 0x55, 1, false, body(32, 33));
    m.push_data_group(r1.data(), r1.size());
    auto r2 = group(4, 0x55, 2, true, body(10, 65));
    CHECK(m.push_data_group(r2.data(), r2.size()));
    CHECK(m.image_size() == 74);
}
