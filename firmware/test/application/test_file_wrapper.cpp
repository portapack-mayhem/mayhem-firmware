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
        if (offset_ + bytes_to_read >= size())
            bytes_to_read = size() - offset_;
        
        if (bytes_to_read == 0 || bytes_to_read >= size()) // NB: underflow wrap
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
                CHECK(f.size() == 0);
            }
        }

        GIVEN("Not empty string") {
            MockFile f{"abc"};

            THEN("size() should be string length.") {
                CHECK(f.size() == 3);
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
                    CHECK(r.error().code() == FR_BAD_SEEK);
                }
            }

            WHEN("seek() offset is size()") {
                auto r = f.seek(f.size());
                
                THEN("Result should be bad_seek") {
                    CHECK(r.is_error());
                    CHECK(r.error().code() == FR_BAD_SEEK);
                }
            }

            WHEN("seek() offset > size()") {
                auto r = f.seek(f.size() + 1);
                
                THEN("Result should be bad_seek") {
                    CHECK(r.is_error());
                    CHECK(r.error().code() == FR_BAD_SEEK);
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
                    CHECK(*r == 1);
                }
            }
        }
    }

    SCENARIO("File read") {
        GIVEN("Valid file") {
            MockFile f{"abc\ndef"};

            const auto buf_len = 10;
            std::string buf;
            buf.reserve(buf_len);
            
            WHEN("Reading") {
                auto r = f.read(&buf[0], 3);
                
                THEN("Result should be number of bytes read") {
                    CHECK(r);
                    CHECK(*r == 3);
                }

                THEN("Buffer should contain read data") {
                    CHECK(buf.length() == 3);
                    CHECK(buf == "abc");
                }
            }
        }
    }
}

TEST_SUITE_BEGIN("Test BufferWrapper");

TEST_CASE("It can wrap a MockFile.") {
    MockFile file{""};
    auto wrapped = wrap_buffer(file);
}

TEST_SUITE_END();