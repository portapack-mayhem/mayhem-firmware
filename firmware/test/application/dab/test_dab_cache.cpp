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

#include "dab/dab_cache.hpp"

#include <string>

namespace {

dab::EnsembleDatabase build_db() {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Test Mux");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");
    db.on_subchannel(5, 100, 96, dab::ProtectionProfile{dab::ProtectionKind::Eep, 3, false});
    return db;
}

}  // namespace

TEST_CASE("escaping round trips commas and backslashes") {
    const std::string tricky = "Rock, Pop \\ Jazz";
    const std::string escaped = dab::escape_field(tricky);
    CHECK(escaped.find("\\,") != std::string::npos);
    CHECK(dab::unescape_field(escaped) == tricky);
}

TEST_CASE("a playable service serialises to one line") {
    auto db = build_db();
    std::string out;
    CHECK(dab::serialise_ensemble(db, "12B", 1000, out) == 1);
    CHECK(out.find("12B") == 0);
    CHECK(out.find("Radio One") != std::string::npos);
    CHECK(out.back() == '\n');
}

TEST_CASE("serialise and parse round trip preserves every field") {
    auto db = build_db();
    std::string out;
    REQUIRE(dab::serialise_ensemble(db, "12B", 4242, out) == 1);

    dab::CachedService cs;
    REQUIRE(dab::parse_cache_line(out.c_str(), cs));

    CHECK(cs.block == "12B");
    CHECK(cs.eid == 0x1234);
    CHECK(cs.ensemble_label == "Test Mux");
    CHECK(cs.sid == 0xC0FF);
    CHECK(cs.service_label == "Radio One");
    CHECK(cs.subch_id == 5);
    CHECK(cs.start_cu == 100);
    CHECK(cs.size_cu == 96);
    CHECK(cs.protection.level == 3);
    CHECK_FALSE(cs.protection.option_b);
    CHECK(cs.bitrate == 128000);
    CHECK(cs.is_dab_plus);
    CHECK(cs.last_seen == 4242);
}

TEST_CASE("labels containing commas survive the round trip") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "News, Talk");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Rock, Pop");
    db.on_subchannel(5, 100, 96, dab::ProtectionProfile{dab::ProtectionKind::Eep, 3, false});

    std::string out;
    REQUIRE(dab::serialise_ensemble(db, "5A", 1, out) == 1);

    dab::CachedService cs;
    REQUIRE(dab::parse_cache_line(out.c_str(), cs));
    CHECK(cs.service_label == "Rock, Pop");
    CHECK(cs.ensemble_label == "News, Talk");
}

TEST_CASE("services that cannot be tuned are not cached") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    // Label but no subchannel.
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio One");

    std::string out;
    CHECK(dab::serialise_ensemble(db, "5A", 1, out) == 0);
    CHECK(out.empty());
}

TEST_CASE("the EEP option survives the round trip") {
    dab::EnsembleDatabase db;
    db.on_ensemble(0x1234, 0, false);
    db.on_label(0x1234, dab::LabelKind::Ensemble, "Mux");
    db.on_service_component(0xC0FF, 5, dab::ComponentKind::MscStreamAudio, dab::kAsctyAac, true);
    db.on_label(0xC0FF, dab::LabelKind::ProgrammeService, "Radio");
    db.on_subchannel(5, 100, 54, dab::ProtectionProfile{dab::ProtectionKind::Eep, 1, true});

    std::string out;
    REQUIRE(dab::serialise_ensemble(db, "5A", 1, out) == 1);

    dab::CachedService cs;
    REQUIRE(dab::parse_cache_line(out.c_str(), cs));
    CHECK(cs.protection.option_b);
    CHECK(cs.protection.level == 1);
}

TEST_CASE("malformed lines are rejected without crashing") {
    dab::CachedService cs;

    CHECK_FALSE(dab::parse_cache_line(nullptr, cs));
    CHECK_FALSE(dab::parse_cache_line("", cs));
    CHECK_FALSE(dab::parse_cache_line("# a comment", cs));
    CHECK_FALSE(dab::parse_cache_line("too,few,fields", cs));
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,abc,96,3,0,128000,1,42", cs));   // non-numeric CU
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,0,3,0,128000,1,42", cs));    // zero size
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,99,100,96,3,0,128000,1,42", cs));  // subch out of range
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,9,0,128000,1,42", cs));   // bad protection level
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,,5,100,96,3,0,128000,1,42", cs));        // empty label
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,3,0,128000,1,42,extra", cs));
}

TEST_CASE("a trailing backslash is rejected") {
    dab::CachedService cs;
    CHECK_FALSE(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,3,0,128000,1,42\\", cs));
}

TEST_CASE("a rejected line leaves the output untouched") {
    dab::CachedService cs;
    cs.block = "sentinel";
    CHECK_FALSE(dab::parse_cache_line("garbage", cs));
    CHECK(cs.block == "sentinel");
}

TEST_CASE("trailing newline and carriage return are tolerated") {
    dab::CachedService cs;
    CHECK(dab::parse_cache_line("12B,4660,Mux,49407,Radio,5,100,96,3,0,128000,1,42\r\n", cs));
    CHECK(cs.bitrate == 128000);
}

TEST_CASE("multiple services produce multiple lines") {
    dab::EnsembleDatabase db = build_db();
    db.on_service_component(0xD0FF, 6, dab::ComponentKind::MscStreamAudio, dab::kAsctyMp2, true);
    db.on_label(0xD0FF, dab::LabelKind::ProgrammeService, "Radio Two");
    db.on_subchannel(6, 200, 72, dab::ProtectionProfile{dab::ProtectionKind::Eep, 2, false});

    std::string out;
    CHECK(dab::serialise_ensemble(db, "12B", 7, out) == 2);

    size_t newlines = 0;
    for (char c : out)
        if (c == '\n')
            ++newlines;
    CHECK(newlines == 2);
}
