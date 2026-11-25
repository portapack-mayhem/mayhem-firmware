/*
 * Copyright (C) 2026 PortaPack Mayhem contributors
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
#include "dab/dab_charset.hpp"

#include <string>

namespace {

std::string convert(const std::string& in, uint8_t charset) {
    char out[64];
    const size_t n = dab::to_display(reinterpret_cast<const uint8_t*>(in.data()), in.size(), charset, out, sizeof(out));
    return std::string(out, n);
}

}  // namespace

TEST_SUITE_BEGIN("DAB charset");

TEST_CASE("EBU Latin o-acute becomes Latin-1") {
    CHECK(convert("G\x86ra", dab::kCharsetEbuLatin) == "G\xF3" "ra");
    CHECK(convert("Kr\x86l", dab::kCharsetEbuLatin) == "Kr\xF3l");
}

TEST_CASE("EBU Latin Polish letters outside Latin-1 fold to ASCII") {
    /* z-dot, n-acute, a-ogonek, e-ogonek, s-acute, c-acute, z-acute */
    CHECK(convert("\x0E\x1E\xB6\x10\x11\xFC\xFB\xFD", dab::kCharsetEbuLatin) == "Zznaescz");
}

TEST_CASE("UTF-8 Polish text: Latin-1 kept, the rest folded") {
    CHECK(convert("\xC5\xBB\xC3\xB3\xC5\x82\xC4\x87", dab::kCharsetUtf8) == "Z\xF3" "lc");
    CHECK(convert("Pi\xC4\x99kna \xC5\x81\xC3\xB3" "dka", dab::kCharsetUtf8) == "Piekna L\xF3" "dka");
}

TEST_CASE("controls and padding are dropped, unknown code points become ?") {
    CHECK(convert(std::string("ab\0\0", 4), dab::kCharsetEbuLatin) == "ab");
    CHECK(convert("\xE2\x82\xAC 5", dab::kCharsetUtf8) == "E 5");
    CHECK(convert("\xE2\x98\x83", dab::kCharsetUtf8) == "?");
    CHECK(convert(std::string("\x00\x41\x01\x42", 4), dab::kCharsetUcs2) == "Al");
}

TEST_CASE("output never exceeds capacity") {
    char out[4];
    const std::string in = "abcdefgh";
    CHECK(dab::to_display(reinterpret_cast<const uint8_t*>(in.data()), in.size(), dab::kCharsetEbuLatin, out, sizeof(out)) == 4);
}

TEST_SUITE_END();
