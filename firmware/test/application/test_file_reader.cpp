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
#include "mock_file.hpp"
#include "file_reader.hpp"
#include <cstdio>

TEST_SUITE_BEGIN("Test BufferLineReader");

TEST_CASE("It can iterate file lines.") {
    MockFile f{"abc\ndef\nhij"};
    BufferLineReader<MockFile> reader{f};
    int line_count = 0;
    for (const auto& line : reader) {
        printf("Line: %s", line.c_str());
        ++line_count;
    }

    CHECK_EQ(line_count, 3);
}

TEST_CASE("It can iterate multiple times.") {
    MockFile f{"abc\ndef\nhij"};
    BufferLineReader<MockFile> reader{f};
    int line_count = 0;
    int line_count2 = 0;
    for (const auto& line : reader)
        ++line_count;
    for (const auto& line : reader)
        ++line_count2;

    CHECK_EQ(line_count, 3);
    CHECK_EQ(line_count2, 3);
}

TEST_CASE("It can iterate file ending with newline.") {
    MockFile f{"abc\ndef\nhij\n"};
    BufferLineReader<MockFile> reader{f};
    int line_count = 0;
    for (const auto& line : reader) {
        printf("Line: %s", line.c_str());
        ++line_count;
    }

    CHECK_EQ(line_count, 3);
}

TEST_CASE("It can iterate files with empty lines.") {
    MockFile f{"abc\ndef\n\nhij\n\n"};
    BufferLineReader<MockFile> reader{f};
    int line_count = 0;
    for (const auto& line : reader) {
        printf("Line: %s", line.c_str());
        ++line_count;
    }

    CHECK_EQ(line_count, 5);
}

TEST_CASE("It can iterate large lines.") {
    std::string long_line(0x90, 'b');
    long_line.back() = 'c';

    MockFile f{long_line + "\n" + long_line + "\n"};
    BufferLineReader<MockFile> reader{f};
    int line_count = 0;
    for (const auto& line : reader) {
        CHECK_EQ(line.length(), 0x91);
        printf("Line: %s", line.c_str());
        ++line_count;
    }

    CHECK_EQ(line_count, 2);
}

TEST_SUITE_END();

TEST_SUITE_BEGIN("Test split_string");

TEST_CASE("Empty string returns no results.") {
    auto r = split_string("", ',');
    CHECK(r.empty());
}

TEST_CASE("String without delimiter returns 1 result.") {
    auto r = split_string("hello", ',');
    REQUIRE_EQ(r.size(), 1);
    CHECK(r[0] == "hello");
}

TEST_CASE("It will split on delimiter.") {
    auto r = split_string("hello,world", ',');
    REQUIRE_EQ(r.size(), 2);
    CHECK(r[0] == "hello");
    CHECK(r[1] == "world");
}

TEST_CASE("It will return empty columns.") {
    auto r = split_string("hello,,world", ',');
    REQUIRE_EQ(r.size(), 3);
    CHECK(r[0] == "hello");
    CHECK(r[1] == "");
    CHECK(r[2] == "world");
}

TEST_CASE("It will return empty first column.") {
    auto r = split_string(",hello,world", ',');
    REQUIRE_EQ(r.size(), 3);
    CHECK(r[0] == "");
    CHECK(r[1] == "hello");
    CHECK(r[2] == "world");
}

TEST_CASE("It will return empty last column.") {
    auto r = split_string("hello,world,", ',');
    REQUIRE_EQ(r.size(), 3);
    CHECK(r[0] == "hello");
    CHECK(r[1] == "world");
    CHECK(r[2] == "");
}

TEST_CASE("It will split only empty columns.") {
    auto r = split_string(",,,,", ',');
    REQUIRE_EQ(r.size(), 5);
    CHECK(r[0] == "");
    CHECK(r[4] == "");
}

TEST_SUITE_END();

TEST_CASE("count_lines returns 1 for single line") {
    MockFile f{"abs"};
    BufferLineReader<MockFile> reader{f};
    CHECK_EQ(count_lines(reader), 1);
}

TEST_CASE("count_lines returns 2 for 2 lines") {
    MockFile f{"abs"};
    BufferLineReader<MockFile> reader{f};
    CHECK_EQ(count_lines(reader), 1);
}

/* Simple example of how to use this to read settings by lines. */
TEST_CASE("It can parse a settings file.") {
    MockFile f{"100,File.txt,5\n200,File2.txt,7"};
    BufferLineReader<MockFile> reader{f};
    std::vector<std::string> data;

    for (const auto& line : reader) {
        auto cols = split_string(line, ',');
        for (auto col : cols)
            data.emplace_back(col);
    }

    REQUIRE_EQ(data.size(), 6);
    CHECK(data[0] == "100");
    CHECK(data[1] == "File.txt");
    CHECK(data[2] == "5\n");  // NB: Newlines need to be manually trimmed.
    CHECK(data[3] == "200");
    CHECK(data[4] == "File2.txt");
    CHECK(data[5] == "7");
}
