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

#include "dab/dab_scanner.hpp"

#include <cstring>
#include <string>

using dab::Scanner;

namespace {

Scanner::Config default_config() {
    Scanner::Config c;
    c.include_l_band = false;
    c.deep = false;
    c.rssi_threshold = 40;
    c.harvest_ms = 3000;
    return c;
}

/* Runs the prescan to completion, returning strong readings only for the
 * named blocks. */
void run_prescan(Scanner& s, const char* const* strong, size_t strong_count) {
    while (s.phase() == Scanner::Phase::Prescan) {
        const auto* b = s.current_block();
        REQUIRE(b != nullptr);

        uint8_t rssi = 10;
        for (size_t i = 0; i < strong_count; ++i) {
            if (std::strcmp(b->name, strong[i]) == 0)
                rssi = 90;
        }
        s.report_rssi(rssi);
    }
}

}  // namespace

TEST_CASE("a fresh scanner is idle") {
    Scanner s;
    CHECK(s.phase() == Scanner::Phase::Idle);
    CHECK(s.current_block() == nullptr);
    CHECK(s.progress_percent() == 0);
}

TEST_CASE("start begins a prescan over band III") {
    Scanner s;
    s.start(default_config());
    CHECK(s.phase() == Scanner::Phase::Prescan);
    REQUIRE(s.current_block() != nullptr);
    CHECK(std::string(s.current_block()->name) == "5A");
    CHECK(s.current_dwell_ms() == dab::kDefaultPrescanMs);
}

TEST_CASE("prescan walks every band III block in order") {
    Scanner s;
    s.start(default_config());

    size_t visited = 0;
    uint32_t last_freq = 0;
    while (s.phase() == Scanner::Phase::Prescan) {
        const auto* b = s.current_block();
        REQUIRE(b != nullptr);
        CHECK(b->frequency_hz > last_freq);
        last_freq = b->frequency_hz;
        ++visited;
        s.report_rssi(0);
    }
    CHECK(visited == 41);
}

TEST_CASE("a prescan with nothing above threshold ends the search") {
    Scanner s;
    s.start(default_config());
    run_prescan(s, nullptr, 0);

    CHECK(s.phase() == Scanner::Phase::Done);
    CHECK(s.result_count() == 0);
    CHECK(s.progress_percent() == 100);
}

TEST_CASE("only blocks above threshold are harvested") {
    Scanner s;
    s.start(default_config());

    const char* strong[] = {"12B", "11D"};
    run_prescan(s, strong, 2);

    REQUIRE(s.phase() == Scanner::Phase::Harvest);
    CHECK(s.current_dwell_ms() == 3000);

    // Harvest order follows the block table, so 11D comes before 12B.
    REQUIRE(s.current_block() != nullptr);
    CHECK(std::string(s.current_block()->name) == "11D");
    s.report_harvest(true, true, 100);

    REQUIRE(s.current_block() != nullptr);
    CHECK(std::string(s.current_block()->name) == "12B");
    s.report_harvest(true, true, 100);

    CHECK(s.phase() == Scanner::Phase::Done);
    REQUIRE(s.result_count() == 2);
    CHECK(std::string(s.results()[0].block->name) == "11D");
    CHECK(std::string(s.results()[1].block->name) == "12B");
}

TEST_CASE("an early complete ends the dwell without waiting") {
    Scanner s;
    s.start(default_config());
    const char* strong[] = {"12B"};
    run_prescan(s, strong, 1);

    REQUIRE(s.phase() == Scanner::Phase::Harvest);
    // Well short of the 3000 ms dwell, but the ensemble is complete.
    s.report_harvest(true, true, 250);
    CHECK(s.phase() == Scanner::Phase::Done);
    CHECK(s.result_count() == 1);
}

TEST_CASE("an incomplete harvest holds until the dwell expires") {
    Scanner s;
    s.start(default_config());
    const char* strong[] = {"12B"};
    run_prescan(s, strong, 1);

    s.report_harvest(false, false, 100);
    CHECK(s.phase() == Scanner::Phase::Harvest);
    s.report_harvest(false, false, 2999);
    CHECK(s.phase() == Scanner::Phase::Harvest);

    s.report_harvest(false, false, 3000);
    CHECK(s.phase() == Scanner::Phase::Done);
}

TEST_CASE("a block above threshold with no ensemble is not recorded") {
    Scanner s;
    s.start(default_config());
    const char* strong[] = {"12B"};
    run_prescan(s, strong, 1);

    // Strong signal but nothing decodable: an analogue TV carrier, say.
    s.report_harvest(false, false, 3000);
    CHECK(s.phase() == Scanner::Phase::Done);
    CHECK(s.result_count() == 0);
}

TEST_CASE("deep search skips the prescan and harvests every block") {
    Scanner s;
    auto c = default_config();
    c.deep = true;
    s.start(c);

    CHECK(s.phase() == Scanner::Phase::Harvest);
    REQUIRE(s.current_block() != nullptr);
    CHECK(std::string(s.current_block()->name) == "5A");

    size_t harvested = 0;
    while (s.phase() == Scanner::Phase::Harvest) {
        ++harvested;
        s.report_harvest(false, false, 3000);
    }
    CHECK(harvested == 41);
    CHECK(s.phase() == Scanner::Phase::Done);
}

TEST_CASE("L-band is included only when configured") {
    Scanner s;
    auto c = default_config();
    c.include_l_band = true;
    c.deep = true;
    s.start(c);

    size_t harvested = 0;
    while (s.phase() == Scanner::Phase::Harvest) {
        ++harvested;
        s.report_harvest(false, false, 3000);
    }
    CHECK(harvested == 41 + 23);
}

TEST_CASE("progress advances monotonically to 100") {
    Scanner s;
    s.start(default_config());

    uint8_t last = 0;
    const char* strong[] = {"5A", "12B"};

    while (s.phase() != Scanner::Phase::Done) {
        const uint8_t p = s.progress_percent();
        CHECK(p >= last);
        CHECK(p <= 100);
        last = p;

        if (s.phase() == Scanner::Phase::Prescan) {
            const auto* b = s.current_block();
            uint8_t rssi = 10;
            for (const char* n : strong)
                if (std::strcmp(b->name, n) == 0)
                    rssi = 90;
            s.report_rssi(rssi);
        } else {
            s.report_harvest(true, true, 100);
        }
    }
    CHECK(s.progress_percent() == 100);
}

TEST_CASE("abort returns the scanner to idle") {
    Scanner s;
    s.start(default_config());
    s.report_rssi(90);
    s.abort();

    CHECK(s.phase() == Scanner::Phase::Idle);
    CHECK(s.current_block() == nullptr);
}

TEST_CASE("reports outside the active phase are ignored") {
    Scanner s;
    // Nothing started yet.
    s.report_rssi(90);
    s.report_harvest(true, true, 0);
    CHECK(s.phase() == Scanner::Phase::Idle);

    s.start(default_config());
    // Harvest reports during prescan must not advance anything.
    const auto* before = s.current_block();
    s.report_harvest(true, true, 5000);
    CHECK(s.current_block() == before);
}

TEST_CASE("a second start resets prior results") {
    Scanner s;
    s.start(default_config());
    const char* strong[] = {"12B"};
    run_prescan(s, strong, 1);
    s.report_harvest(true, true, 10);
    REQUIRE(s.result_count() == 1);

    s.start(default_config());
    CHECK(s.result_count() == 0);
    CHECK(s.phase() == Scanner::Phase::Prescan);
}
