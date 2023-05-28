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
#include "file.hpp"
#include "file_wrapper.hpp"

#include <cstring>
#include <cstdio>
#include <string>

/* Mocks the File interface with a backing string. */
class MockFile {
   public:
    using Error = File::Error;
    using Offset = File::Offset;
    using Size = File::Size;
    template <typename T>
    using Result = File::Result<T>;

    MockFile(std::string data)
        : data_{std::move(data)} {}

    Size size() { return data_.size(); }

    Result<Offset> seek(uint32_t offset) {
        if (offset >= size())
            return {static_cast<Error>(FR_BAD_SEEK)};

        auto previous = offset_;
        offset_ = offset;
        return previous;
    }

    Result<Size> read(void* data, Size bytes_to_read) {
        if (offset_ + bytes_to_read > size())
            bytes_to_read = size() - offset_;

        if (bytes_to_read == 0 || bytes_to_read > size())  // NB: underflow wrap
            return 0;

        memcpy(data, &data_[offset_], bytes_to_read);
        offset_ += bytes_to_read;
        return bytes_to_read;
    }

    std::string data_;
    uint32_t offset_{0};
};

/* Verifies correctness of MockFile. */
TEST_SUITE("Test MockFile") {
    SCENARIO("File size") {
        GIVEN("Empty string") {
            MockFile f{""};

            THEN("size() should be 0.") {
                CHECK_EQ(f.size(), 0);
            }
        }

        GIVEN("Not empty string") {
            MockFile f{"abc"};

            THEN("size() should be string length.") {
                CHECK_EQ(f.size(), 3);
            }
        }
    }

    SCENARIO("File seek") {
        GIVEN("Valid file") {
            MockFile f{"abc\ndef"};

            WHEN("seek() negative offset") {
                auto r = f.seek(-1);

                THEN("Result should be bad_seek") {
                    CHECK(r.is_error());
                    CHECK_EQ(r.error().code(), FR_BAD_SEEK);
                }
            }

            WHEN("seek() offset is size()") {
                auto r = f.seek(f.size());

                THEN("Result should be bad_seek") {
                    CHECK(r.is_error());
                    CHECK_EQ(r.error().code(), FR_BAD_SEEK);
                }
            }

            WHEN("seek() offset > size()") {
                auto r = f.seek(f.size() + 1);

                THEN("Result should be bad_seek") {
                    CHECK(r.is_error());
                    CHECK_EQ(r.error().code(), FR_BAD_SEEK);
                }
            }

            WHEN("seek() offset < size()") {
                auto r = f.seek(1);

                THEN("Result should be ok") {
                    CHECK(r.is_ok());
                }

                r = f.seek(3);

                THEN("Result should be previous offset") {
                    CHECK(r);
                    CHECK_EQ(*r, 1);
                }
            }
        }
    }

    SCENARIO("File read") {
        GIVEN("Valid file") {
            MockFile f{"abc\ndef"};

            const auto buf_len = 10;
            std::string buf;
            buf.resize(buf_len);

            WHEN("Reading") {
                auto r = f.read(&buf[0], 3);

                THEN("Result should be number of bytes read") {
                    CHECK(r);
                    CHECK_EQ(*r, 3);
                }

                buf.resize(*r);
                THEN("Buffer should contain read data") {
                    CHECK_EQ(buf.length(), 3);
                    CHECK_EQ(buf, "abc");
                }

                r = f.read(&buf[0], 3);
                THEN("Reading should continue where it left off") {
                    CHECK_EQ(buf.length(), 3);
                    CHECK_EQ(buf, "\nde");
                }

                r = f.read(&buf[0], 3);
                THEN("Reading should stop at the end of the file") {
                    CHECK(r);
                    CHECK_EQ(*r, 1);

                    buf.resize(*r);
                    CHECK_EQ(buf.length(), 1);
                    CHECK_EQ(buf, "f");
                }
            }

            WHEN("Reading block larger than file size") {
                auto r = f.read(&buf[0], buf_len);
                buf.resize(*r);

                THEN("It should read to file end.") {
                    CHECK(r);
                    CHECK_EQ(*r, 7);
                    CHECK_EQ(buf, f.data_);
                }
            }
        }
    }
}

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

TEST_SUITE_END();