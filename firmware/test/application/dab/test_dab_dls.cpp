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

#include "dab/dab_dls.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> segment(uint8_t toggle, bool first, bool last, uint8_t seg_num, const std::string& text) {
    std::vector<uint8_t> out;
    uint8_t b0 = static_cast<uint8_t>((toggle << 7) | (first ? 0x40 : 0) | (last ? 0x20 : 0));
    b0 |= static_cast<uint8_t>((text.size() - 1) & 0x0F);
    out.push_back(b0);
    out.push_back(first ? 0x00 : static_cast<uint8_t>((seg_num & 0x07) << 4));
    for (char c : text)
        out.push_back(static_cast<uint8_t>(c));
    return out;
}

}  // namespace

TEST_CASE("a single-segment label is published") {
    dab::DlsAssembler d;
    auto s = segment(0, true, true, 0, "Now Playing");
    CHECK(d.push_segment(s.data(), s.size()));
    CHECK(std::string(d.text()) == "Now Playing");
    CHECK(d.take_new_text());
    CHECK_FALSE(d.take_new_text());
}

TEST_CASE("a three-segment label is joined in order") {
    dab::DlsAssembler d;
    auto s0 = segment(0, true, false, 0, "The Beatles ");
    auto s1 = segment(0, false, false, 1, "- Hey Jude");
    auto s2 = segment(0, false, true, 2, " (1968)");

    CHECK_FALSE(d.push_segment(s0.data(), s0.size()));
    CHECK_FALSE(d.push_segment(s1.data(), s1.size()));
    CHECK(d.push_segment(s2.data(), s2.size()));

    CHECK(std::string(d.text()) == "The Beatles - Hey Jude (1968)");
}

TEST_CASE("segments arriving out of order still reassemble") {
    dab::DlsAssembler d;
    auto s0 = segment(0, true, false, 0, "AAAA");
    auto s1 = segment(0, false, false, 1, "BBBB");
    auto s2 = segment(0, false, true, 2, "CCCC");

    d.push_segment(s2.data(), s2.size());
    d.push_segment(s1.data(), s1.size());
    CHECK(d.push_segment(s0.data(), s0.size()));

    CHECK(std::string(d.text()) == "AAAABBBBCCCC");
}

TEST_CASE("a missing middle segment publishes nothing") {
    dab::DlsAssembler d;
    auto s0 = segment(0, true, false, 0, "AAAA");
    auto s2 = segment(0, false, true, 2, "CCCC");

    CHECK_FALSE(d.push_segment(s0.data(), s0.size()));
    CHECK_FALSE(d.push_segment(s2.data(), s2.size()));
    CHECK(std::string(d.text()).empty());
}

TEST_CASE("a flipped toggle replaces rather than appends") {
    dab::DlsAssembler d;
    auto a = segment(0, true, true, 0, "First Song");
    d.push_segment(a.data(), a.size());
    REQUIRE(std::string(d.text()) == "First Song");

    auto b = segment(1, true, true, 0, "Second");
    CHECK(d.push_segment(b.data(), b.size()));
    CHECK(std::string(d.text()) == "Second");
}

TEST_CASE("segments from the previous toggle are discarded") {
    dab::DlsAssembler d;
    auto old0 = segment(0, true, false, 0, "OLD ");
    d.push_segment(old0.data(), old0.size());

    // New label starts; the stale segment 0 must not survive into it.
    auto new0 = segment(1, true, false, 0, "NEW ");
    auto new1 = segment(1, false, true, 1, "TEXT");
    d.push_segment(new0.data(), new0.size());
    CHECK(d.push_segment(new1.data(), new1.size()));
    CHECK(std::string(d.text()) == "NEW TEXT");
}

TEST_CASE("a clear-display command empties the text") {
    dab::DlsAssembler d;
    auto s = segment(0, true, true, 0, "Something");
    d.push_segment(s.data(), s.size());
    REQUIRE(!std::string(d.text()).empty());

    const uint8_t clear[2] = {0x11, 0x00};  // command flag set, code 1
    CHECK(d.push_segment(clear, sizeof(clear)));
    CHECK(std::string(d.text()).empty());
}

TEST_CASE("a truncated segment is refused") {
    dab::DlsAssembler d;
    const uint8_t bad[2] = {0x6F, 0x00};  // claims 16 chars, supplies none
    CHECK_FALSE(d.push_segment(bad, sizeof(bad)));
    CHECK_FALSE(d.push_segment(nullptr, 8));
    CHECK_FALSE(d.push_segment(bad, 1));
}

TEST_CASE("reset clears state") {
    dab::DlsAssembler d;
    auto s = segment(0, true, true, 0, "Text");
    d.push_segment(s.data(), s.size());
    d.reset();
    CHECK(std::string(d.text()).empty());
    CHECK_FALSE(d.take_new_text());
}

TEST_CASE("text never exceeds the maximum label length") {
    dab::DlsAssembler d;
    for (uint8_t i = 0; i < 8; ++i) {
        const bool first = (i == 0);
        const bool last = (i == 7);
        auto s = segment(0, first, last, i, "0123456789ABCDEF");
        d.push_segment(s.data(), s.size());
    }
    CHECK(std::strlen(d.text()) <= dab::kMaxDlsChars);
}
