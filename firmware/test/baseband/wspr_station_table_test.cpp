/*
 * Copyright (C) 2026 Ivan
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

/* Host tests for the WSPR RX fixed station table model
 * (ui_station_table.hpp), exercised through the same header the device UI
 * will use. Covers: capacity/eviction, one record per callsign,
 * unique-vs-duplicate N accounting with TX-identity tolerances, N
 * saturation, the session-persistent confirmation contract (payload change
 * re-bases, silence never un-confirms), agreement as a display-only metric
 * (every accepted report visible), age as a separate property, CLEAR
 * LIST. */

#include "ui_station_table.hpp"

#include <doctest.h>

#include <cstring>
#include <string>

using namespace ui::external_app::wspr_rx;
using stations::EmptyState;
using stations::StationRecord;
using stations::StationTable;
using stations::WSPR_STATION_MAX;

namespace {

/* Feed a report; unique-ness is returned so callers can assert on it. */
bool feed(StationTable& t, const char* call, const char* grid, int8_t dbm, int16_t freq, int8_t snr, uint8_t agreement, uint32_t now_s, uint32_t start_br, int16_t col, const char* selected = nullptr) {
    return t.report(call, grid, dbm, freq, snr, agreement, now_s, start_br, col,
                    selected);
}

/* N helper. */
uint16_t n_of(const StationTable& t, const char* call) {
    const StationRecord* r = t.find(call);
    return r ? r->n_reports : 0;
}

/* 13 distinct stations' worth of identity: each new call is a whole WSPR
 * slot period (352 block-rows) away from the previous one. */
constexpr uint32_t BR_STEP = 352;

}  // namespace

TEST_CASE("station table: 0/1/5/6/12/13 unique stations, order and eviction") {
    SUBCASE("zero stations: empty state is no-reports") {
        StationTable t;
        CHECK(t.count() == 0);
        CHECK(t.visible_count() == 0);
        CHECK(t.empty_state() == EmptyState::NoReports);
        CHECK(t.record(0) == nullptr);
        CHECK(t.visible_record(0) == nullptr);
    }

    SUBCASE("1 and 5 stations: first-appearance order, updates stay in place") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 20, 1400, -5, 90, 0, 1000, 50));
        // Repeat of the same station in a later window stays at index 0.
        CHECK(feed(t, "A1AAA", "JN49", 20, 1401, -6, 88, 120, 1000 + BR_STEP, 50));
        for (unsigned i = 1; i < 5; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "B2BB%u", i);
            CHECK(feed(t, call, "LO10", 27, 1400 + i * 10, 3, 85, i * 120,
                       1000 + i * BR_STEP, 60));
        }
        CHECK(t.count() == 5);
        CHECK(t.visible_count() == 5);
        CHECK(std::string(t.record(0)->callsign) == "A1AAA");
        CHECK(std::string(t.record(4)->callsign) == "B2BB4");
        // In-place update: latest fields changed, position did not.
        CHECK(t.record(0)->snr_db == -6);
        CHECK(t.record(0)->agreement_pct == 88);
        CHECK(t.record(0)->n_reports == 2);
        CHECK(std::string(t.record(0)->grid) == "JN49");
    }

    SUBCASE("6 and 12 stations fit without eviction") {
        StationTable t;
        for (unsigned i = 0; i < 6; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "C3CC%u", i);
            CHECK(feed(t, call, "JO59", 30, 1200 + i * 8, 0, 70, i * 10,
                       500 + i * BR_STEP, 40));
        }
        CHECK(t.count() == 6);
        for (unsigned i = 6; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "D4DD%u", i);
            CHECK(feed(t, call, "JO59", 30, 1200 + i * 8, 0, 70, i * 10,
                       500 + i * BR_STEP, 40));
        }
        CHECK(t.count() == WSPR_STATION_MAX);
        CHECK(t.find("C3CC0") != nullptr);
        CHECK(t.find("D4DD11") != nullptr);
    }

    SUBCASE("13th station evicts the least-recently-updated, keeps order") {
        StationTable t;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "E5EE%02u", i);
            feed(t, call, "JM89", 17, 1300, 5, 90, i + 1, 700 + i * BR_STEP, 45);
        }
        CHECK(t.count() == 12);
        // E5EE00 is the least recently updated (t=1). A repeat of E5EE05
        // makes IT recent, so the next eviction victim is E5EE01, not E5EE05.
        CHECK(feed(t, "E5EE05", "JM89", 17, 1301, 6, 91, 100, 700 + 5 * BR_STEP + BR_STEP, 45));
        CHECK(feed(t, "F6FF00", "JM89", 17, 1302, 6, 91, 101, 700 + 12 * BR_STEP, 45));
        CHECK(t.count() == 12);
        CHECK(t.find("E5EE00") == nullptr);  // LRU evicted
        CHECK(t.find("E5EE01") != nullptr);  // untouched
        CHECK(t.find("E5EE05") != nullptr);  // recently updated, kept
        CHECK(t.find("F6FF00") != nullptr);  // newest, appended last
        // First-appearance order of survivors is preserved.
        CHECK(std::string(t.record(0)->callsign) == "E5EE01");
        CHECK(std::string(t.record(11)->callsign) == "F6FF00");
    }

    SUBCASE("eviction never removes the currently selected callsign") {
        StationTable t;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "G7GG%02u", i);
            feed(t, call, "JN58", 10, 1350, -2, 80, i + 1, 800 + i * BR_STEP, 30);
        }
        // G7GG00 is the LRU and also the user's selected row.
        CHECK(feed(t, "H8HH00", "JN58", 10, 1351, -2, 80, 100, 800 + 12 * BR_STEP,
                   30, "G7GG00"));
        CHECK(t.count() == 12);
        CHECK(t.find("G7GG00") != nullptr);  // selected: protected
        CHECK(t.find("G7GG01") == nullptr);  // next LRU paid the price
        CHECK(t.find("H8HH00") != nullptr);
    }
}

TEST_CASE("station table: one record per callsign, dedup tolerances, N saturation") {
    SUBCASE("unique reports increment N, record count stays one") {
        StationTable t;
        uint32_t br = 2000;
        for (unsigned i = 1; i <= 5; i++) {
            CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, i * 120, br, 70));
            br += BR_STEP;
        }
        CHECK(t.count() == 1);
        CHECK(n_of(t, "A1AAA") == 5);
    }

    SUBCASE("same-TX re-delivery within tolerance never increments N") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 0, 5000, 70));
        // Tolerance boundaries (br +-4, col +-6) are all the SAME TX:
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 2, 5000 + 4, 70 + 6));
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 2, 5000 - 4, 70 - 6));
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 2, 5000, 70 + 6));
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 2, 5000 + 1, 74));
        CHECK(t.count() == 1);
        CHECK(n_of(t, "A1AAA") == 1);
        // Latest fields untouched by duplicates (snr/agreement still t=0's).
        const StationRecord* r = t.find("A1AAA");
        CHECK(r->snr_db == -10);
        CHECK(r->agreement_pct == 75);
        CHECK(r->last_report_s == 0);
    }

    SUBCASE("just past the boundary is a new TX") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 0, 5000, 70));
        // br gap 5 (past +-4): unique even with col equal.
        CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 120, 5005, 70));
        // col gap 7 (past +-6) at the SAME onset: the ambiguous one-packet
        // case - not a new TX, uncertainty recorded instead (policy: only a
        // new onset can establish independence).
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 240, 5005, 77));
        CHECK(n_of(t, "A1AAA") == 2);
        CHECK(t.find("A1AAA")->n_ambiguous == 1);
        // col gap 7 at a genuinely new onset: unique.
        CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 360, 5005 + BR_STEP, 77));
        CHECK(t.count() == 1);
        CHECK(n_of(t, "A1AAA") == 3);
    }

    SUBCASE("N saturates at the documented 999") {
        StationTable t;
        uint32_t br = 3000;
        // Many windows: N climbs to 999 and stops there.
        for (unsigned i = 1; i <= 1001; i++) {
            feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, i, br, 70);
            br += BR_STEP;
        }
        CHECK(n_of(t, "A1AAA") == 999);
        CHECK(n_of(t, "A1AAA") == stations::WSPR_N_MAX);
        const StationRecord* r = t.find("A1AAA");
        // Rendering-ready extremes: 3 characters cover 999.
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%u", (unsigned)r->n_reports);
        CHECK(std::string(buf) == "999");
    }

    SUBCASE("distinct callsigns never merge") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -10, 75, 0, 1000, 50));
        CHECK(feed(t, "A1AAB", "JN49", 30, 1500, -10, 75, 120, 1000 + BR_STEP, 50));
        CHECK(t.count() == 2);
        CHECK(n_of(t, "A1AAA") == 1);
        CHECK(n_of(t, "A1AAB") == 1);
    }
}

TEST_CASE("station table: confirmation contract and persistent session state") {
    SUBCASE("first report is an unconfirmed candidate, independent TX confirms") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        CHECK_FALSE(t.record(0)->confirmed);
        CHECK_FALSE(t.is_confirmed(*t.record(0)));
        // A genuinely new physical TX (whole slot period later) repeating the
        // SAME payload: confirmed.
        CHECK(feed(t, "A1AAA", "JN49", 33, 1501, -9, 91, 120, 4000 + BR_STEP, 60));
        CHECK(t.record(0)->confirmed);
        CHECK(t.is_confirmed(*t.record(0)));
        CHECK(n_of(t, "A1AAA") == 2);
    }

    SUBCASE("payload change updates latest fields without false confirmation") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        CHECK(feed(t, "A1AAA", "JN49", 33, 1501, -9, 91, 120, 4000 + BR_STEP, 60));
        CHECK(t.record(0)->confirmed);
        // Same callsign, DIFFERENT payload (grid + power changed): the latest
        // fields adopt it, but it must NOT confirm the new payload by itself.
        CHECK(feed(t, "A1AAA", "JO59", 27, 1502, -7, 92, 240, 4000 + 2 * BR_STEP, 60));
        const StationRecord* r = t.record(0);
        CHECK(std::string(r->grid) == "JO59");
        CHECK(r->dbm == 27);
        CHECK_FALSE(r->confirmed);
        CHECK_FALSE(t.is_confirmed(*r));
        CHECK(n_of(t, "A1AAA") == 3);  // still counted as a unique report
    }

    SUBCASE("a power-only payload change also re-bases to unconfirmed") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 120, 4000 + BR_STEP, 60));
        REQUIRE(t.record(0)->confirmed);
        // Same grid and frequency, DIFFERENT power: new payload, no inherited
        // confirmation.
        CHECK(feed(t, "A1AAA", "JN49", 30, 1500, -8, 90, 240, 4000 + 2 * BR_STEP, 60));
        CHECK(t.record(0)->dbm == 30);
        CHECK_FALSE(t.record(0)->confirmed);
        CHECK_FALSE(t.is_confirmed(*t.record(0)));
    }

    SUBCASE("confirmation persists through silence far beyond the acquisition TTL") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 120, 4000 + BR_STEP, 60));
        REQUIRE(t.record(0)->confirmed);
        REQUIRE(t.age_s(*t.record(0), 120) == 0);

        // No new report at all, only time passing: the age keeps growing while
        // the session confirmation is unchanged (no TTL can clear it).
        const uint32_t later = 120 + (12u * 60u) + 1000;
        CHECK(t.age_s(*t.record(0), later) ==
              (12u * 60u) + 1000);
        CHECK(t.is_confirmed(*t.record(0)));
        CHECK(t.effective_confirmed(*t.record(0)));
        CHECK(t.visible_confirmed(0));
        CHECK(t.find("A1AAA")->n_reports == 2);

        // A repeat of the SAME payload after the TTL is still the confirmed
        // payload; it refreshes the age and never un-confirms.
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, later,
                   4000 + 2 * BR_STEP, 60));
        CHECK(t.record(0)->confirmed);
        CHECK(t.age_s(*t.record(0), later) == 0);
        CHECK(t.find("A1AAA")->n_reports == 3);
    }

    SUBCASE("independent repeat confirms even after a long silence") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        // An independent matching packet confirms even after a day of silence.
        const uint32_t late = 24u * 60u * 60u;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, late,
                   4000 + BR_STEP, 60));
        CHECK(t.record(0)->confirmed);
        // Further independent repeats preserve confirmation.
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, late + 120,
                   4000 + 2 * BR_STEP, 60));
        CHECK(t.record(0)->confirmed);
    }

    SUBCASE("confirmation identity tracks the confirming payload, not the call") {
        StationTable t;
        CHECK(feed(t, "B2BBB", "LO10", 20, 1400, 0, 60, 0, 9000, 10));
        // A repeat confirms regardless of audio-frequency drift.
        CHECK(feed(t, "B2BBB", "LO10", 20, 1403, 0, 60, 120, 9000 + BR_STEP, 10));
        CHECK(t.record(0)->confirmed);
        // A frequency jump preserves confirmation of the same payload.
        CHECK(feed(t, "B2BBB", "LO10", 20, 1500, 0, 60, 240, 9000 + 2 * BR_STEP, 10));
        CHECK(t.record(0)->confirmed);
    }
}

TEST_CASE("station table: every accepted report is visible (agreement is display-only)") {
    StationTable t;
    // station A: unconfirmed, A% = 0
    CHECK(feed(t, "A1AAA", "JN49", 30, 1400, -5, 0, 0, 1000, 50));
    // station B: unconfirmed, A% = 5
    CHECK(feed(t, "B2BBB", "JN49", 30, 1410, -5, 5, 10, 1000 + BR_STEP, 50));
    // station C: unconfirmed, A% = 80
    CHECK(feed(t, "C3CCC", "JN49", 30, 1420, -5, 80, 20, 1000 + 2 * BR_STEP, 50));
    // station D: unconfirmed, A% = 100
    CHECK(feed(t, "D4DDD", "JN49", 30, 1430, -5, 100, 30, 1000 + 3 * BR_STEP, 50));
    // station E: confirmed via an independent TX of the same payload
    CHECK(feed(t, "E5EEE", "JN49", 30, 1440, -5, 1, 40, 1000 + 4 * BR_STEP, 50));
    CHECK(feed(t, "E5EEE", "JN49", 30, 1440, -5, 1, 160, 1000 + 5 * BR_STEP, 50));
    CHECK(t.record(4)->confirmed);

    // No threshold hides anything: low-agreement and confirmed records are
    // all accepted reports and all visible, in first-appearance order.
    CHECK(t.visible_count() == 5);
    CHECK(t.empty_state() == EmptyState::Reports);
    for (size_t i = 0; i < 5; i++)
        CHECK(t.visible_record(i) == t.record(i));
    CHECK(t.find("A1AAA")->agreement_pct == 0);
    CHECK(t.find("D4DDD")->agreement_pct == 100);
}

TEST_CASE("station table: clear and re-feed keeps every report visible") {
    StationTable t;
    // A very low agreement and a confirmed record coexist; both stay visible.
    CHECK(feed(t, "F6FFF", "JN49", 30, 1400, -5, 0, 0, 2000, 50));
    CHECK(feed(t, "G7GGG", "JN49", 30, 1410, -5, 1, 10, 2000 + BR_STEP, 50));
    CHECK(feed(t, "G7GGG", "JN49", 30, 1410, -5, 1, 130, 2000 + 2 * BR_STEP, 50));
    REQUIRE(t.find("G7GGG")->confirmed);
    CHECK(t.count() == 2);
    CHECK(t.visible_count() == 2);
    CHECK(t.empty_state() == EmptyState::Reports);

    // A later unique report can never move a record out of the view.
    CHECK(feed(t, "F6FFF", "JN49", 30, 1400, -5, 0, 200, 2000 + 3 * BR_STEP, 50));
    CHECK(t.visible_count() == 2);
    CHECK(t.find("F6FFF")->n_reports == 2);
}

TEST_CASE("station table: long callsigns and max rendering values") {
    StationTable t;
    // 12-character compound callsign, stored exactly and terminated.
    const char* long_call = "ZZ9XYZW1AWQ0";
    CHECK(std::strlen(long_call) <= stations::WSPR_CALL_MAX);
    CHECK(feed(t, long_call, "AA00", 39, 1600, 39, 100, 0, 11000, 90));
    const StationRecord* r = t.find(long_call);
    REQUIRE(r != nullptr);
    CHECK(std::string(r->callsign) == long_call);
    CHECK(std::strlen(r->callsign) == std::strlen(long_call));
    // Callsign storage is always a valid C string.
    CHECK(r->callsign[stations::WSPR_CALL_MAX] == '\0');
    // A 12-char call that is truncated at insertion still matches itself
    // (bounded storage: identical prefix, one record).
    char overlong[20];
    std::snprintf(overlong, sizeof(overlong), "%sXX", long_call);
    CHECK(feed(t, overlong, "AA00", 39, 1600, 39, 100, 120, 11000 + BR_STEP, 90));
    CHECK(t.count() == 1);  // same bounded key, no record multiplication
    // Max rendering values.
    CHECK(r->n_reports == 2);
    CHECK(r->snr_db == 39);
    CHECK(r->agreement_pct == 100);
    CHECK(std::string(r->grid) == "AA00");
    CHECK(r->dbm == 39);
    CHECK(r->freq_hz == 1600);
    CHECK(r->last_valid);
    CHECK(r->last_report_s == 120);
}

TEST_CASE("station table: confirmation display survives time; age grows separately") {
    SUBCASE("confirmed record after a long silence: still confirmed, age grows") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 120, 4000 + BR_STEP, 60));
        REQUIRE(t.record(0)->confirmed);
        REQUIRE(t.visible_count() == 1);

        // Just one second later the age is 1 s.
        CHECK(t.age_s(*t.record(0), 121) == 1);
        // Past the old 12-minute TTL horizon, with NO new report: the
        // confirmation is unchanged and only the age advanced.
        const uint32_t late = 120 + (12u * 60u) + 1;
        CHECK(t.age_s(*t.record(0), late) == (12u * 60u) + 1);
        CHECK(t.effective_confirmed(*t.record(0)));
        CHECK(t.is_confirmed(*t.record(0)));
        CHECK(t.visible_confirmed(0));
        // Record, N and visibility all survive.
        CHECK(t.count() == 1);
        CHECK(t.visible_count() == 1);
        CHECK(t.find("A1AAA")->n_reports == 2);
        CHECK(t.empty_state() == EmptyState::Reports);
    }

    SUBCASE("an unconfirmed record stays grey while its age grows") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
        CHECK_FALSE(t.effective_confirmed(*t.record(0)));
        CHECK_FALSE(t.visible_confirmed(0));
        CHECK(t.age_s(*t.record(0), (12u * 60u) + 5) ==
              (12u * 60u) + 5);
        CHECK_FALSE(t.effective_confirmed(*t.record(0)));
        CHECK(t.visible_count() == 1);
    }

    SUBCASE("time never deletes records mid-table, confirmed flags included") {
        StationTable t;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "M1MM%02u", i);
            feed(t, call, "JM89", 17, 1300, 5, 90, i + 1, 700 + i * BR_STEP, 45);
        }
        // Confirm M1MM06 in place (repeat payload, new TX).
        CHECK(feed(t, "M1MM06", "JM89", 17, 1300, 5, 90, 200, 700 + 6 * BR_STEP + BR_STEP, 45));
        CHECK(t.record(6)->confirmed);
        CHECK(t.count() == 12);
        CHECK(t.visible_count() == 12);

        // A very old clock: every record survives and the confirmed one is
        // still displayed as confirmed (no TTL greying).
        const uint32_t late = (12u * 60u) + 201;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "M1MM%02u", i);
            CHECK(t.find(call) != nullptr);
        }
        CHECK(t.count() == 12);
        CHECK(t.visible_count() == 12);
        CHECK(t.effective_confirmed(*t.record(6)));
        CHECK(t.age_s(*t.record(6), late) == late - 200);
    }
}

TEST_CASE("station table: CLEAR LIST then the next report appears fresh") {
    StationTable t;
    CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 0, 4000, 60));
    CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 120, 4000 + BR_STEP, 60));
    CHECK(feed(t, "B2BBB", "LO10", 20, 1400, 0, 50, 130, 9000, 10));
    CHECK(t.count() == 2);
    CHECK(t.record(0)->confirmed);
    CHECK(t.visible_count() == 2);

    t.reset();
    // Nothing stale: counts, records, confirmation and display state gone.
    CHECK(t.count() == 0);
    CHECK(t.visible_count() == 0);
    CHECK(t.find("A1AAA") == nullptr);
    CHECK(t.find("B2BBB") == nullptr);
    CHECK(t.empty_state() == EmptyState::NoReports);
    CHECK(t.visible_record(0) == nullptr);

    // The next report starts from scratch: N=1, unconfirmed, first in order.
    CHECK(feed(t, "A1AAA", "JN49", 33, 1500, -8, 90, 1000, 12000, 60));
    CHECK(t.count() == 1);
    CHECK(n_of(t, "A1AAA") == 1);
    CHECK_FALSE(t.record(0)->confirmed);
    CHECK(std::string(t.record(0)->grid) == "JN49");
    CHECK(t.record(0)->last_report_s == 1000);
    // Every accepted report is visible after a clear too.
    CHECK(t.visible_count() == 1);
    CHECK(t.empty_state() == EmptyState::Reports);
}

TEST_CASE("station table: same-onset column-separated decode is uncertainty, not confirmation") {
    SUBCASE("one packet re-decoded ~99 Hz apart within seconds: no N, no confirm") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 20, 1500, -5, 90, 0, 1000, 50));
        CHECK_FALSE(t.visible_confirmed(0));
        // Same physical onset (start_br within the +/-4 tolerance) but a
        // tone-0 column ~68 bins away (~99 Hz) - the observed one-packet
        // two-frequency signature. The metadata cannot prove two
        // independent transmissions, so this is recorded as ambiguous.
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 20, 1599, -8, 85, 5, 1003, 118));
        const StationRecord* r = t.find("A1AAA");
        CHECK(r->n_reports == 1);
        CHECK(r->n_ambiguous == 1);
        CHECK_FALSE(t.visible_confirmed(0));
        // The ambiguous observation changes nothing else: latest fields,
        // freshness anchor and confirmation identity stay untouched.
        CHECK(r->freq_hz == 1500);
        CHECK(r->last_report_s == 0);
        CHECK(std::string(r->grid) == "JN49");
        CHECK(r->dbm == 20);
    }

    SUBCASE("an ambiguous duplicate never erases an established confirmation") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 20, 1500, -5, 90, 0, 1000, 50));
        CHECK(feed(t, "A1AAA", "JN49", 20, 1501, -6, 88, 120, 1000 + BR_STEP, 51));
        CHECK(t.visible_confirmed(0));
        // A later same-onset re-decode at another column: uncertainty only.
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 20, 1599, -9, 84, 240, 1000 + BR_STEP + 1, 118));
        const StationRecord* r = t.find("A1AAA");
        CHECK(r->n_reports == 2);
        CHECK(r->n_ambiguous == 1);
        CHECK(t.visible_confirmed(0));
    }

    SUBCASE("a frequency change alone never makes the station suspicious") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 20, 1500, -5, 90, 0, 1000, 50));
        CHECK(feed(t, "A1AAA", "JN49", 20, 1501, -6, 88, 120, 1000 + BR_STEP, 51));
        CHECK(t.visible_confirmed(0));
        // Established station moves 100 Hz in a later window: frequency is
        // display-only, so the repeat is still a confirming independent TX.
        CHECK(feed(t, "A1AAA", "JN49", 20, 1600, -7, 87, 240, 1000 + 2 * BR_STEP, 118));
        CHECK(t.visible_confirmed(0));
        CHECK(t.find("A1AAA")->freq_hz == 1600);
    }

    SUBCASE("small drift inside the same-TX tolerance: deduped, neither suspicious nor independent") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 20, 1400, -5, 90, 0, 1000, 50));
        // Drifted re-delivery of the SAME TX: onset within tolerance and
        // column within tolerance - the existing dedup, untouched.
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 20, 1407, -6, 88, 5, 1001, 53));
        const StationRecord* r = t.find("A1AAA");
        CHECK(r->n_reports == 1);
        CHECK(r->n_ambiguous == 0);
        CHECK_FALSE(t.visible_confirmed(0));
    }

    SUBCASE("duplicate delivery of one payload twice in one window counts once") {
        StationTable t;
        CHECK(feed(t, "A1AAA", "JN49", 20, 1500, -5, 90, 0, 2000, 70));
        // Same col hypothesis:
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 20, 1500, -5, 90, 2, 2001, 70));
        // Cross-column hypothesis of the same packet:
        CHECK_FALSE(feed(t, "A1AAA", "JN49", 20, 1599, -8, 85, 4, 2002, 138));
        CHECK(n_of(t, "A1AAA") == 1);
        CHECK(t.find("A1AAA")->n_ambiguous == 1);
        CHECK_FALSE(t.visible_confirmed(0));
    }
}
