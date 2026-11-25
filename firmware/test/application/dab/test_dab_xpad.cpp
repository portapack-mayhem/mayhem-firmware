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

#include "dab/dab_xpad.hpp"
#include "dab/dab_crc.hpp"

#include <algorithm>

#include <cstdint>
#include <string>
#include <vector>

namespace {

struct Recorder : dab::XpadSink {
    std::vector<std::vector<uint8_t>> dls;
    std::vector<std::vector<uint8_t>> mot;

    void on_dls_group(const uint8_t* d, size_t n) override {
        dls.emplace_back(d, d + n);
    }
    void on_mot_group(const uint8_t* d, size_t n) override {
        mot.emplace_back(d, d + n);
    }
};

/* The PAD region as EN 300 401 7.4 stores it: the logical X-PAD (contents
 * indicators, then subfields in order) byte-reversed, then the F-PAD. */
std::vector<uint8_t> stored(std::vector<uint8_t> logical, uint8_t fpad0, uint8_t fpad1) {
    std::reverse(logical.begin(), logical.end());
    logical.push_back(fpad0);
    logical.push_back(fpad1);
    return logical;
}

std::vector<uint8_t> variable_pad(uint8_t app, uint8_t length_index, const std::vector<uint8_t>& payload) {
    const size_t len = dab::xpad_subfield_length(length_index);
    std::vector<uint8_t> x{static_cast<uint8_t>((length_index << 5) | (app & 0x1F)), 0x00};
    for (size_t i = 0; i < len; i++)
        x.push_back(i < payload.size() ? payload[i] : 0);
    return stored(x, 0x20, 0x02);  // variable X-PAD, contents indicator present
}

std::vector<uint8_t> short_pad(uint8_t app, const std::vector<uint8_t>& three) {
    std::vector<uint8_t> x{static_cast<uint8_t>(app & 0x1F)};
    for (size_t i = 0; i < 3; i++)
        x.push_back(i < three.size() ? three[i] : 0);
    return stored(x, 0x10, 0x02);  // short X-PAD, contents indicator present
}

/* A complete DLS segment: prefix, characters, CRC. */
std::vector<uint8_t> dls_segment(const std::string& text) {
    std::vector<uint8_t> g;
    g.push_back(static_cast<uint8_t>(0x40 | 0x20 | ((text.size() - 1) & 0x0F)));  // first+last
    g.push_back(0x00);
    for (char c : text)
        g.push_back(static_cast<uint8_t>(c));
    const uint16_t crc = static_cast<uint16_t>(dab::crc16_ccitt(g.data(), g.size()) ^ 0xFFFF);
    g.push_back(static_cast<uint8_t>(crc >> 8));
    g.push_back(static_cast<uint8_t>(crc & 0xFF));
    return g;
}

/* A continuation frame: no contents indicator, every byte is data. */
std::vector<uint8_t> short_continuation(const std::vector<uint8_t>& four) {
    std::vector<uint8_t> x;
    for (size_t i = 0; i < 4; i++)
        x.push_back(i < four.size() ? four[i] : 0);
    return stored(x, 0x10, 0x00);  // short X-PAD, CI flag clear
}

}  // namespace

TEST_CASE("subfield lengths follow the standard table") {
    const size_t expected[8] = {4, 6, 8, 12, 16, 24, 32, 48};
    for (uint8_t i = 0; i < 8; i++)
        CHECK(dab::xpad_subfield_length(i) == expected[i]);
    // Out-of-range indices wrap rather than read past the table.
    CHECK(dab::xpad_subfield_length(200) == expected[0]);
}

TEST_CASE("no X-PAD indicator produces nothing but is not an error") {
    dab::XpadParser p;
    Recorder r;
    const uint8_t pad[2] = {0x00, 0x00};
    CHECK(p.push_pad(pad, sizeof(pad), r));
    CHECK(r.dls.empty());
    CHECK(r.mot.empty());
}

TEST_CASE("the reserved indicator is rejected") {
    dab::XpadParser p;
    Recorder r;
    const uint8_t pad[4] = {0, 0, 0x30, 0x00};
    CHECK_FALSE(p.push_pad(pad, sizeof(pad), r));
}

TEST_CASE("a short X-PAD DLS subfield starts a group without emitting it") {
    dab::XpadParser p;
    Recorder r;
    // Declares one character, so the group needs 5 bytes and 3 is not enough.
    auto pad = short_pad(dab::kXpadAppDlsStart, {0x60, 0x00, 'X'});
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));
    CHECK(r.dls.empty());
}

TEST_CASE("a variable X-PAD DLS subfield is delivered whole") {
    dab::XpadParser p;
    Recorder r;
    auto group = dls_segment("Hello world!");  // 2 + 12 + 2 = 16 bytes
    auto pad = variable_pad(dab::kXpadAppDlsStart, 4 /* 16 bytes */, group);
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));

    REQUIRE(r.dls.size() == 1);
    CHECK(r.dls[0] == group);
}

TEST_CASE("two subfields in one frame are read in order") {
    dab::XpadParser p;
    Recorder r;
    auto group = dls_segment("Hi!!");  // 8 bytes: a 4-byte start and a 4-byte continuation
    std::vector<uint8_t> x{static_cast<uint8_t>((0 << 5) | dab::kXpadAppDlsStart),
                           static_cast<uint8_t>((0 << 5) | dab::kXpadAppDlsContinue), 0x00};
    x.insert(x.end(), group.begin(), group.end());
    auto pad = stored(x, 0x20, 0x02);
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));

    REQUIRE(r.dls.size() == 1);
    CHECK(r.dls[0] == group);
}

TEST_CASE("a DLS group that fails its CRC is dropped") {
    dab::XpadParser p;
    Recorder r;
    auto group = dls_segment("Hello world!");
    group[5] ^= 0x01;
    auto pad = variable_pad(dab::kXpadAppDlsStart, 4, group);
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));
    CHECK(r.dls.empty());
}

TEST_CASE("MOT subfields route to the MOT sink, not DLS") {
    dab::XpadParser p;
    Recorder r;

    // A length indicator announces the group that follows.
    auto len_pad = variable_pad(dab::kXpadAppLengthIndicator, 0 /* 4 bytes */, {0x00, 12, 0, 0});
    REQUIRE(p.push_pad(len_pad.data(), len_pad.size(), r));

    auto pad = variable_pad(dab::kXpadAppMotStart, 3 /* 12 bytes */, {1, 2, 3, 4});
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));

    CHECK(r.dls.empty());
    REQUIRE(r.mot.size() == 1);
    CHECK(r.mot[0].size() == 12);
}

TEST_CASE("an MOT group without a length indicator waits for the next start") {
    dab::XpadParser p;
    Recorder r;

    auto first = variable_pad(dab::kXpadAppMotStart, 3, {1, 2, 3, 4});
    REQUIRE(p.push_pad(first.data(), first.size(), r));
    CHECK(r.mot.empty());

    auto second = variable_pad(dab::kXpadAppMotStart, 3, {5, 6, 7, 8});
    REQUIRE(p.push_pad(second.data(), second.size(), r));
    CHECK(r.mot.size() == 1);
}

TEST_CASE("a continuation without a start is discarded") {
    dab::XpadParser p;
    Recorder r;
    auto pad = variable_pad(dab::kXpadAppDlsContinue, 2, {9, 9, 9});
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));
    CHECK(r.dls.empty());
}

TEST_CASE("an unknown application type is ignored") {
    dab::XpadParser p;
    Recorder r;
    auto pad = variable_pad(7 /* not DLS or MOT */, 2, {1, 2, 3});
    REQUIRE(p.push_pad(pad.data(), pad.size(), r));
    CHECK(r.dls.empty());
    CHECK(r.mot.empty());
}

TEST_CASE("a subfield larger than the window is skipped, not fatal") {
    dab::XpadParser p;
    Recorder r;
    // Length index 7 is 48 bytes, but only a few are supplied. Subfields sit
    // before the indicators, so the oversized one falls outside the window and
    // is skipped rather than discarding the frame.
    auto pad = stored({static_cast<uint8_t>((7 << 5) | dab::kXpadAppDlsStart), 0x00, 1, 2, 3}, 0x20, 0x02);
    CHECK_FALSE(p.push_pad(pad.data(), pad.size(), r));
    CHECK(r.dls.empty());
}

TEST_CASE("a continuation frame takes only its subfield, not the whole window") {
    // The M4 ships a fixed 128-byte trailing window, so a continuation frame
    // that appended all of it would splice audio payload into the group.
    dab::XpadParser p;
    Recorder r;

    auto group = dls_segment("Hi there!!");  // 14 bytes
    auto start = short_pad(dab::kXpadAppDlsStart, {group[0], group[1], group[2]});
    REQUIRE(p.push_pad(start.data(), start.size(), r));

    // A wide window whose trailing four bytes are the real continuation and
    // whose leading bytes are unrelated payload.
    std::vector<uint8_t> logical(62, 0xEE);
    for (size_t i = 0; i < 4; i++)
        logical[i] = group[3 + i];
    auto wide = stored(logical, 0x10, 0x00);  // short X-PAD, CI flag clear

    REQUIRE(p.push_pad(wide.data(), wide.size(), r));

    // Only four bytes were taken, so the group is still short of its declared
    // length and nothing has been emitted from padding bytes.
    CHECK(r.dls.empty());
}

TEST_CASE("a continuation frame carries no contents indicator") {
    // This is the normal shape of every frame after the first: the CI flag is
    // clear and all four bytes continue the open subfield. Reading a CI here
    // would eat a data byte and throw the rest away, so no label would ever
    // complete.
    dab::XpadParser p;
    Recorder r;

    auto group = dls_segment("Hi there!!");  // prefix 2 + 10 chars + CRC 2 = 14
    REQUIRE(group.size() == 14);

    auto start = short_pad(dab::kXpadAppDlsStart, {group[0], group[1], group[2]});
    REQUIRE(p.push_pad(start.data(), start.size(), r));
    CHECK(r.dls.empty());

    for (size_t offset = 3; offset < group.size(); offset += 4) {
        std::vector<uint8_t> chunk(group.begin() + offset,
                                   group.begin() + std::min(offset + 4, group.size()));
        chunk.resize(4, 0);
        auto cont = short_continuation(chunk);
        REQUIRE(p.push_pad(cont.data(), cont.size(), r));
    }

    REQUIRE(r.dls.size() == 1);
    CHECK(r.dls[0][2] == 'H');
}

TEST_CASE("a too-short region is refused") {
    dab::XpadParser p;
    Recorder r;
    const uint8_t pad[1] = {0};
    CHECK_FALSE(p.push_pad(pad, sizeof(pad), r));
    CHECK_FALSE(p.push_pad(nullptr, 8, r));
}

TEST_CASE("reset drops a partially assembled group") {
    dab::XpadParser p;
    Recorder r;
    auto pad = short_pad(dab::kXpadAppDlsStart, {0x6F, 0x00, 'A'});
    p.push_pad(pad.data(), pad.size(), r);
    p.reset();

    auto cont = short_pad(dab::kXpadAppDlsContinue, {'B', 'C', 'D'});
    REQUIRE(p.push_pad(cont.data(), cont.size(), r));
    // The continuation has nothing to attach to after the reset.
    CHECK(r.dls.empty());
}

TEST_CASE("group assembly is bounded and cannot overrun") {
    dab::XpadParser p;
    Recorder r;

    // Far more data than a data group may hold. Declaring 16 characters keeps
    // the group from completing, so it grows until the bound stops it.
    auto start = variable_pad(dab::kXpadAppMotStart, 7, {});
    p.push_pad(start.data(), start.size(), r);

    for (int i = 0; i < 40; i++) {
        auto pad = variable_pad(dab::kXpadAppMotContinue, 7, {});
        p.push_pad(pad.data(), pad.size(), r);
    }

    for (const auto& g : r.mot)
        CHECK(g.size() <= dab::kMaxDataGroupBytes);
}
