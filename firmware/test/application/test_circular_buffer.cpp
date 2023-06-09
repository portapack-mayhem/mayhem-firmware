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
#include "circular_buffer.hpp"

TEST_SUITE_BEGIN("circular buffer");

SCENARIO("Items can be pushed and popped from front and back.") {
    GIVEN("an empty buffer") {
        CircularBuffer<int, 5> cb;

        REQUIRE(cb.empty());
        REQUIRE(cb.size() == 0);

        WHEN("push_back()") {
            cb.push_back(1);

            THEN("size should increase") {
                CHECK(cb.empty() == false);
                CHECK(cb.size() == 1);
            }

            cb.push_back(2);

            THEN("size should increase") {
                CHECK(cb.empty() == false);
                CHECK(cb.size() == 2);
            }

            THEN("back() should be the last item pushed") {
                CHECK(cb.back() == 2);
            }

            THEN("front() should be the first item pushed") {
                CHECK(cb.front() == 1);
            }
        }

        WHEN("push_front()") {
            cb.push_front(3);

            THEN("size should increase") {
                CHECK(cb.empty() == false);
                CHECK(cb.size() == 1);
            }

            cb.push_front(4);

            THEN("size should increase") {
                CHECK(cb.empty() == false);
                CHECK(cb.size() == 2);
            }

            THEN("back() should be first item pushed") {
                CHECK(cb.back() == 3);
            }

            THEN("front() should be the last item pushed") {
                CHECK(cb.front() == 4);
            }
        }

        WHEN("pop_back()") {
            cb.pop_back();

            THEN("size should not change") {
                REQUIRE(cb.empty());
                REQUIRE(cb.size() == 0);
            }
        }

        WHEN("pop_front()") {
            cb.pop_back();

            THEN("size should not change") {
                REQUIRE(cb.empty());
                REQUIRE(cb.size() == 0);
            }
        }
    }

    GIVEN("a buffer with items") {
        CircularBuffer<int, 5> cb;
        cb.push_back(1);
        cb.push_back(2);
        cb.push_back(3);

        REQUIRE(!cb.empty());
        REQUIRE(cb.size() == 3);
        REQUIRE(cb.front() == 1);
        REQUIRE(cb.back() == 3);

        WHEN("pop_back()") {
            cb.pop_back();

            THEN("size should decrease") {
                CHECK(cb.empty() == false);
                CHECK(cb.size() == 2);
            }

            THEN("back item should be removed") {
                CHECK(cb.back() == 2);
            }

            THEN("front item should be unchanged") {
                CHECK(cb.front() == 1);
            }
        }

        WHEN("pop_front()") {
            cb.pop_front();

            THEN("size should decrease") {
                CHECK(cb.empty() == false);
                CHECK(cb.size() == 2);
            }

            THEN("front item should be removed") {
                CHECK(cb.front() == 2);
            }

            THEN("back item should be unchanged") {
                CHECK(cb.back() == 3);
            }
        }

        WHEN("clear()") {
            cb.clear();

            THEN("size should be empty") {
                CHECK(cb.empty());
                CHECK(cb.size() == 0);
            }
        }
    }

    GIVEN("a full buffer") {
        CircularBuffer<int, 4> cb;
        cb.push_back(1);
        cb.push_back(2);
        cb.push_back(3);

        REQUIRE(!cb.empty());
        REQUIRE(cb.size() == 3);
        REQUIRE(cb.front() == 1);
        REQUIRE(cb.back() == 3);

        WHEN("push_back()") {
            cb.push_back(4);

            THEN("size should not be changed") {
                CHECK(!cb.empty());
                CHECK(cb.size() == 3);
            }

            THEN("front should be popped") {
                CHECK(cb.front() == 2);
            }

            THEN("back should be new value") {
                CHECK(cb.back() == 4);
            }
        }

        WHEN("push_front()") {
            cb.push_front(4);

            THEN("size should not be changed") {
                CHECK(!cb.empty());
                CHECK(cb.size() == 3);
            }

            THEN("front should be new value") {
                CHECK(cb.front() == 4);
            }

            THEN("back should be popped") {
                CHECK(cb.back() == 2);
            }
        }
    }
}

SCENARIO("Items can be accessed randomly") {
    GIVEN("buffer with items") {
        CircularBuffer<int, 4> cb;
        cb.push_front(1);
        cb.push_back(2);
        cb.push_back(3);

        WHEN("accessing items") {
            THEN("front should be at 0") {
                CHECK(cb.front() == cb[0]);
            }

            THEN("last should be at size() - 1") {
                CHECK(cb.back() == cb[cb.size() - 1]);
            }

            THEN("all should be accessible") {
                // Assumes values are in order.
                for (size_t i = 0; i < cb.size(); i++)
                    CHECK(cb[i] == i + 1);
            }
        }

        WHEN("accessing items after push_front") {
            cb.push_front(4);

            THEN("front should be at 0") {
                CHECK(cb.front() == cb[0]);
            }

            THEN("last should be at size() - 1") {
                CHECK(cb.back() == cb[cb.size() - 1]);
            }
        }

        WHEN("accessing items after push_back") {
            cb.push_back(4);

            THEN("front should be at 0") {
                CHECK(cb.front() == cb[0]);
            }

            THEN("last should be at size() - 1") {
                CHECK(cb.back() == cb[cb.size() - 1]);
            }
        }
    }
}

TEST_SUITE_END();