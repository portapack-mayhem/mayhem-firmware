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
#include "file.hpp"
#include "file_wrapper.hpp"
#include "mock_file.hpp"

TEST_SUITE_BEGIN("Test BufferWrapper");

TEST_CASE("It can wrap a MockFile.") {
    MockFile f{""};
    auto w = wrap_buffer(f);
}

SCENARIO("Empty file") {
    GIVEN("An empty file") {
        MockFile f{""};
        auto w = wrap_buffer(f);

        WHEN("Initializing") {
            CHECK_EQ(w.size(), 0);
            REQUIRE_EQ(w.line_count(), 1);
        }

        WHEN("Getting line_range()") {
            auto r = w.line_range(0);
            CHECK(r);
            CHECK_EQ(r->start, 0);
            CHECK_EQ(r->end, 1);
        }

        WHEN("Getting line_length()") {
            CHECK_EQ(w.line_length(0), 1);
        }
    }
}

SCENARIO("Basic file") {
    GIVEN("A file") {
        MockFile f{"abc\ndef"};
        auto w = wrap_buffer(f);

        WHEN("Initializing") {
            CHECK_EQ(w.size(), 7);
            REQUIRE_EQ(w.line_count(), 2);
        }

        WHEN("Getting line_range()") {
            auto r = w.line_range(0);
            CHECK(r);
            CHECK_EQ(r->start, 0);
            CHECK_EQ(r->end, 4);

            r = w.line_range(1);
            CHECK(r);
            CHECK_EQ(r->start, 4);
            CHECK_EQ(r->end, 7);
        }

        WHEN("Getting line_length()") {
            CHECK_EQ(w.line_length(0), 4);
            CHECK_EQ(w.line_length(1), 3);
        }
    }
}

SCENARIO("Reading file lines.") {
    GIVEN("A valid file") {
        MockFile f{"abc\ndef"};
        auto w = wrap_buffer(f);

        WHEN("Reading a line") {
            auto str = w.get_text(0, 0, 10);

            THEN("It should read exactly one line.") {
                REQUIRE(str);
                CHECK_EQ(str->length(), 4);  // Includes '\n'
                CHECK_EQ(*str, "abc\n");
            }
        }

        WHEN("Reading the last line") {
            auto str = w.get_text(w.line_count() - 1, 0, 10);

            THEN("It should read exactly one line.") {
                REQUIRE(str);
                CHECK_EQ(str->length(), 3);
                CHECK_EQ(*str, "def");
            }
        }

        WHEN("Reading past the last line") {
            auto str = w.get_text(w.line_count(), 0, 10);

            THEN("It should return empty value.") {
                REQUIRE(!str);
            }
        }
    }
}

SCENARIO("Reading with cache miss.") {
    GIVEN("A valid file") {
        MockFile f{"abc\ndef\nghi\njkl\nmno"};
        constexpr uint32_t cache_size = 2;
        auto w = wrap_buffer<cache_size>(f);

        CHECK_EQ(w.start_line(), 0);

        WHEN("Reading a cached line") {
            auto str = w.get_text(0, 0, 10);

            THEN("It should read exactly one line.") {
                REQUIRE(str);
                CHECK_EQ(*str, "abc\n");
            }
        }

        WHEN("Reading line after last cached line.") {
            auto str = w.get_text(w.line_count() - 1, 0, 10);

            THEN("It should read exactly one line.") {
                REQUIRE(str);
                CHECK_EQ(*str, "mno");
            }

            THEN("It should push cache window forward to include line.") {
                CHECK_EQ(w.start_line(), w.line_count() - cache_size);
            }
        }

        WHEN("Reading line before first cached line.") {
            // First move cache forward to end.
            w.get_text(w.line_count() - 1, 0, 10);
            auto str = w.get_text(1, 0, 10);

            THEN("It should read exactly one line.") {
                REQUIRE(str);
                CHECK_EQ(*str, "def\n");
            }

            THEN("It should push cache window backward to include line.") {
                CHECK_EQ(w.start_line(), 1);
            }
        }

        WHEN("Reading line 0 before first cached line.") {
            // First move cache forward to end, then back to beginning.
            w.get_text(w.line_count() - 1, 0, 10);
            auto str = w.get_text(0, 0, 10);

            THEN("It should read exactly one line.") {
                REQUIRE(str);
                CHECK_EQ(*str, "abc\n");
            }

            THEN("It should push cache window backward to include line.") {
                CHECK_EQ(w.start_line(), 0);
            }
        }
    }
}

SCENARIO("Replace range of same size.") {
    GIVEN("A file with lines") {
        MockFile f{"abc\ndef"};
        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Replacing range without changing size") {
            w.replace_range({0, 3}, "xyz");

            CHECK_EQ("xyz\ndef", f.data_);

            THEN("size should not change.") {
                CHECK_EQ(w.size(), init_size);
            }

            THEN("text should be replaced.") {
                auto str = w.get_text(0, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "xyz\n");
            }
        }
    }
}

SCENARIO("Replace range that increases size.") {
    GIVEN("A file with lines") {
        MockFile f{"abc\ndef"};
        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Replacing range with larger size") {
            w.replace_range({0, 3}, "wxyz");

            CHECK_EQ(f.data_, "wxyz\ndef");

            THEN("size should be increased.") {
                CHECK_EQ(w.size(), init_size + 1);
            }

            THEN("text should be replaced.") {
                auto str = w.get_text(0, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "wxyz\n");
            }

            THEN("following text should not be modified.") {
                auto str = w.get_text(1, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "def");
            }
        }
    }

    GIVEN("A file larger than internal buffer_size (512)") {
        std::string content = std::string(599, 'a');
        content.push_back('x');
        MockFile f{content};

        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Replacing range with larger size") {
            w.replace_range({0, 2}, "bbb");

            THEN("size should be increased.") {
                CHECK_EQ(w.size(), init_size + 1);
            }

            THEN("text should be replaced.") {
                auto str = w.get_text(0, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "bbbaaaaaaa");
            }

            THEN("end text should be preserved.") {
                CHECK_EQ(f.data_.back(), 'x');
            }
        }
    }
}

SCENARIO("Replace range that decreases size.") {
    GIVEN("A file with lines") {
        MockFile f{"abc\ndef"};
        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Replacing range with smaller size") {
            w.replace_range({0, 3}, "yz");

            CHECK_EQ(f.data_, "yz\ndef");

            THEN("size should be decreased.") {
                CHECK_EQ(w.size(), init_size - 1);
            }

            THEN("text should be replaced.") {
                auto str = w.get_text(0, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "yz\n");
            }

            THEN("following text should not be modified.") {
                auto str = w.get_text(1, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "def");
            }
        }
    }

    GIVEN("A file larger than internal buffer_size (512)") {
        std::string content = std::string(599, 'a');
        content.push_back('x');
        MockFile f{content};

        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Replacing range with smaller size") {
            w.replace_range({0, 10}, "b");

            THEN("size should be decreased.") {
                CHECK_EQ(w.size(), init_size - 9);
            }

            THEN("text should be replaced.") {
                auto str = w.get_text(0, 0, 10);
                REQUIRE(str);
                CHECK_EQ(*str, "baaaaaaaaa");
            }

            THEN("end should be moved toward front.") {
                CHECK_EQ(f.data_.back(), 'x');
            }
        }
    }
}

SCENARIO("Insert line.") {
    GIVEN("A file with lines") {
        MockFile f{"abc\ndef\nghi\n"};
        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Inserting before the first line") {
            w.insert_line(0);

            THEN("should increment line count and size.") {
                CHECK_EQ(w.line_count(), init_line_count + 1);
                CHECK_EQ(w.size(), init_size + 1);
            }

            THEN("should insert empty line content.") {
                auto str = w.get_text(0, 0, 10);
                CHECK_EQ(*str, "\n");
            }

            THEN("first line should be pushed down.") {
                auto str = w.get_text(1, 0, 10);
                CHECK_EQ(*str, "abc\n");
            }
        }

        WHEN("Inserting before the last line") {
            w.insert_line(2);

            THEN("should increment line count and size.") {
                CHECK_EQ(w.line_count(), init_line_count + 1);
                CHECK_EQ(w.size(), init_size + 1);
            }

            THEN("should insert empty line content.") {
                auto str = w.get_text(2, 0, 10);
                CHECK_EQ(*str, "\n");
            }

            THEN("last line should be pushed down.") {
                auto str = w.get_text(3, 0, 10);
                CHECK_EQ(*str, "ghi\n");
            }
        }

        WHEN("Inserting after the last line") {
            w.insert_line(w.line_count());

            THEN("should increment line count and size.") {
                CHECK_EQ(w.line_count(), init_line_count + 1);
                CHECK_EQ(w.size(), init_size + 1);
            }

            THEN("should insert empty line content.") {
                auto str = w.get_text(3, 0, 10);
                CHECK_EQ(*str, "\n");
            }
        }
    }
}

SCENARIO("Delete line.") {
    GIVEN("A file with lines") {
        MockFile f{"abc\ndef\nghi"};
        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();

        WHEN("Deleting the first line") {
            w.delete_line(0);

            THEN("should decrement line count and size.") {
                CHECK_EQ(w.line_count(), init_line_count - 1);
                CHECK_EQ(w.size(), init_size - 4);
            }

            THEN("should remove first line content.") {
                auto str = w.get_text(0, 0, 10);
                CHECK_EQ(*str, "def\n");
            }
        }

        WHEN("Deleting the middle line") {
            w.delete_line(1);

            THEN("should decrement line count and size.") {
                CHECK_EQ(w.line_count(), init_line_count - 1);
                CHECK_EQ(w.size(), init_size - 4);
            }

            THEN("should remove middle line content.") {
                auto str = w.get_text(1, 0, 10);
                CHECK_EQ(*str, "ghi");
            }
        }

        WHEN("Deleting the last line") {
            w.delete_line(2);

            THEN("should decrement line count and size.") {
                CHECK_EQ(w.line_count(), init_line_count - 1);
                CHECK_EQ(w.size(), init_size - 3);
            }

            THEN("should remove last line content.") {
                auto str = w.get_text(2, 0, 10);
                CHECK(!str);
            }
        }
    }
}

SCENARIO("It calls on_read_progress while reading.") {
    GIVEN("A file larger than internal buffer_size (512)") {
        std::string content = std::string(599, 'a');
        content.push_back('x');
        MockFile f{content};

        auto w = wrap_buffer(f);
        auto init_line_count = w.line_count();
        auto init_size = w.size();
        auto called = false;

        w.on_read_progress = [&called](auto, auto) {
            called = true;
        };

        WHEN("Replacing range with larger size") {
            w.replace_range({0, 2}, "bbb");

            THEN("callback should be called.") {
                CHECK(called);
            }
        }
    }
}

TEST_SUITE_END();