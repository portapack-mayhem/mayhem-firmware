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
            auto init_size = f.size();

            WHEN("seek()") {
                f.seek(4);
                THEN("offset_ should be updated.") {
                    CHECK_EQ(f.offset_, 4);
                }
            }

            WHEN("seek() negative offset") {
                auto r = f.seek(-1);

                THEN("Result should be bad_seek.") {
                    CHECK(r.is_error());
                    CHECK_EQ(r.error().code(), FR_BAD_SEEK);
                }
            }

            WHEN("seek() offset is size()") {
                auto r = f.seek(f.size());

                THEN("File should not grow.") {
                    CHECK(r.is_ok());
                    CHECK_EQ(f.size(), init_size);
                }
            }

            WHEN("seek() offset > size()") {
                auto r = f.seek(f.size() + 1);

                THEN("File should grow.") {
                    CHECK(r.is_ok());
                    CHECK_EQ(f.size(), init_size + 1);
                }
            }

            WHEN("seek() offset < size()") {
                auto r = f.seek(1);

                THEN("Result should be ok.") {
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

    SCENARIO("File write") {
        GIVEN("Valid file") {
            MockFile f{"abc\ndef"};

            WHEN("Writing over existing region") {
                f.write("xyz", 3);

                THEN("It should overwrite") {
                    CHECK_EQ(f.data_, "xyz\ndef");
                }
            }

            WHEN("Writing over past end") {
                f.seek(f.size());
                f.write("xyz", 3);

                THEN("It should extend file and write") {
                    CHECK_EQ(f.size(), 10);
                    CHECK_EQ(f.data_, "abc\ndefxyz");
                }
            }
        }
    }

    // This scenario was tested on device.
    SCENARIO("File truncate") {
        GIVEN("Valid file") {
            MockFile f{"hello world"};

            WHEN("truncating at offset 5") {
                f.seek(5);
                f.truncate();

                THEN("resulting file should be 'hello'.") {
                    CHECK_EQ(f.size(), 5);
                    CHECK_EQ(f.data_, "hello");
                }
            }
        }
    }

    SCENARIO("File truncate") {
        GIVEN("Valid file") {
            MockFile f{"abc\ndef"};
            auto init_size = f.size();

            WHEN("R/W pointer at end") {
                f.seek(f.size());
                f.truncate();

                THEN("It should not change size.") {
                    CHECK_EQ(f.size(), init_size);
                }
            }

            WHEN("R/W pointer in middle") {
                f.seek(3);
                f.truncate();

                THEN("It should change size.") {
                    CHECK_EQ(f.size(), 3);
                }
            }
        }
    }
}
