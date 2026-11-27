/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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
#include "external_app_layout.hpp"

TEST_CASE("External app memory banks and limits") {
    external_app::Layout l;
    CHECK(external_app::layout(3, 0xa004, 0x2000, l));
    CHECK(l.m4_base == 0x10080000);
    CHECK(l.m4_size == 0x8000);
    CHECK(l.ui_base == 0x10088000);
    CHECK(l.ui_base + l.ui_size == 0x1008a000);
    CHECK_FALSE(external_app::layout(3, 0xa008, 0x2000, l));
    CHECK(external_app::layout(3, 0x1004, 0, l));
    CHECK(l.ui_base == 0x10080000);
    CHECK(l.m4_size == 0);

    CHECK(external_app::layout(4, 96 * 1024 + 4, 8 * 1024, l));
    CHECK(l.m4_base == 0x10000000);
    CHECK(l.m4_size == 88 * 1024);
    CHECK(l.ui_base == 0x10088000);
    CHECK(l.ui_base + l.ui_size == 0x1008a000);
    CHECK_FALSE(external_app::layout(4, 96 * 1024 + 8, 8 * 1024, l));
    CHECK_FALSE(external_app::layout(4, 96 * 1024 + 4, 8 * 1024 + 4, l));
    CHECK_FALSE(external_app::layout(4, 1024, 0, l));
    CHECK(external_app::layout(3, 64 * 1024 + 4, 4096, l, 64 * 1024));
    CHECK(l.ui_base + l.ui_size == 0x10090000);
    CHECK_FALSE(external_app::layout(3, 64 * 1024 + 8, 4096, l, 64 * 1024));
}

TEST_CASE("Reject truncated or malformed external images before RAM writes") {
    external_app::Layout l;
    CHECK_FALSE(external_app::layout(4, 0x100000400ULL, 84, l));
    CHECK_FALSE(external_app::layout(5, 1024, 84, l));
    CHECK_FALSE(external_app::layout(3, 80, 0, l));
    CHECK_FALSE(external_app::layout(3, 1023, 84, l));
    CHECK_FALSE(external_app::layout(4, 1024, 85, l));
    CHECK_FALSE(external_app::layout(4, 1024, 80, l));
    CHECK_FALSE(external_app::layout(4, 1024, 1020, l));
    CHECK_FALSE(external_app::layout(4, 1024, 0xfffffffc, l));
    CHECK_FALSE(external_app::layout(4, 0xfffffffc, 84, l));
}
