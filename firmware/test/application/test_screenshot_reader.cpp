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
#include "screenshot_reader.hpp"

#include <cstdio>

TEST_CASE("Can open test file.") {
    FILE* f = fopen("SCR_0000.PNG", "r");
    REQUIRE(f);
    fclose(f);
}

TEST_CASE("It should accept screenshot file.") {
    int lines_recv = 0;

    ScreenshotReader reader;
    reader.on_line = [&lines_recv](auto line_pixels) {
        lines_recv++;
    };

    FILE* f = fopen("SCR_0000.PNG", "r");
    constexpr uint32_t buffer_size = 64;
    uint8_t buffer[buffer_size];

    while (true) {
        auto read = fread(buffer, sizeof(buffer[0]), buffer_size, f);
        auto valid = reader.parse(buffer, read);

        if (read < buffer_size || !valid)
            break;
    }

    CHECK_EQ(lines_recv, 320);

    fclose(f);
}