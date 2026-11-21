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
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* Host tests for the WSPR RX diagnostic screen state (WSPR_RX_DIAG): the
 * operation/last/metrics state machine and its fixed-buffer formatters,
 * exercised through the same header the device UI uses. */

#include "wspr_diag.hpp"
#include "ui_station_table.hpp"

#include <doctest.h>

#include <cstdio>
#include <cstring>
#include <string>

using namespace ui::external_app::wspr_rx;
using diag::CandidateState;
using diag::ConfirmTable;
using diag::DiagState;
using diag::format_candidate_panel;
using diag::format_last;
using diag::format_operation;
using diag::LastResult;
using diag::Operation;
using diag::TelemetrySnapshot;

namespace {

TelemetrySnapshot capture_snapshot(uint16_t rows, uint32_t captured_samples, uint8_t windows = 1) {
    TelemetrySnapshot s;
    s.phase = TelemetrySnapshot::PHASE_CAPTURE;
    s.valid_flags = TelemetrySnapshot::VALID_ROWS | TelemetrySnapshot::VALID_CAPTURED;
    s.rows = rows;
    s.captured_samples = captured_samples;
    s.window_count = windows;
    return s;
}

}  // namespace

TEST_CASE("before any telemetry and analysis the screen shows WAIT and LAST --") {
    DiagState st;
    char buf[diag::LINE_CHARS + 1];

    CHECK(st.operation(0) == Operation::WaitM4);
    CHECK(st.operation(3600) == Operation::WaitM4);
    CHECK(st.last() == LastResult::None);

    format_operation(st, 3600, "SCANNING", buf);
    CHECK(std::string(buf) == "SCANNING WAIT M4");
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: --");
}

TEST_CASE("CAPTURE line carries rows and captured seconds") {
    DiagState st;
    st.on_telemetry(capture_snapshot(42, 42 * 8192));
    char buf[diag::LINE_CHARS + 1];
    format_operation(st, 1, "SCANNING", buf);
    /* Rows are block-rows, 0..351 (352 block-rows = one full ring). */
    CHECK(std::string(buf) == "SCANNING r42/351 FEC idle");
}

TEST_CASE("capture then decode then empty analysis yields NO DECODE") {
    DiagState st;
    st.on_telemetry(capture_snapshot(176, 1441792));

    // Window full, decode starts.
    TelemetrySnapshot dec = capture_snapshot(176, 1441792);
    dec.phase = TelemetrySnapshot::PHASE_DECODE;
    dec.valid_flags = TelemetrySnapshot::VALID_ROWS;
    st.on_telemetry(dec);
    CHECK(st.operation(0) == Operation::Decode);

    // Analysis finished: nothing decoded, probe found nothing either.
    TelemetrySnapshot t = capture_snapshot(0, 0);
    t.valid_flags = TelemetrySnapshot::VALID_DECODE_MS;
    t.last_decode_ms = 1533;
    st.on_telemetry(t);
    st.on_window_analysis(0, -1, 0);

    CHECK(st.last() == LastResult::NoDecode);
    char buf[diag::LINE_CHARS + 1];
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: NO DECODE");
    // UI v1: the decode wall time moved into the CAPTURE row's FEC state:
    // idle while the telemetry phase is capture...
    format_operation(st, 2, "SCANNING", buf);
    CHECK(std::string(buf).find("FEC idle") != std::string::npos);
    // ...busy (with the last decode wall time) while it is decode.
    TelemetrySnapshot d2 = capture_snapshot(0, 0);
    d2.valid_flags = TelemetrySnapshot::VALID_DECODE_MS | TelemetrySnapshot::VALID_ROWS;
    d2.phase = TelemetrySnapshot::PHASE_DECODE;
    d2.last_decode_ms = 1533;
    st.on_telemetry(d2);
    format_operation(st, 2, "SCANNING", buf);
    CHECK(std::string(buf).find("FEC busy") != std::string::npos);
    CHECK(std::string(buf).find("1533") != std::string::npos);  // may truncate "ms" at the 29-char line budget
}

TEST_CASE("sync candidate requires the probe acceptance threshold") {
    DiagState st;
    // Probe below the decoder's own acceptance threshold: stays NO DECODE.
    st.on_window_analysis(0, 13, 3);
    CHECK(st.last() == LastResult::NoDecode);
    CHECK_FALSE(st.probe_valid());
    // A genuine anchor (score >= the probe's WSPR_PROBE_MIN_EXCESS).
    st.on_window_analysis(0, 13, 8);
    CHECK(st.last() == LastResult::SyncCand);
    CHECK(st.probe_valid());
    CHECK(st.probe_row() == 13);
    CHECK(st.probe_score() == 8);
    // The LAST line prints the stored analysis anchor WITHOUT needing any
    // telemetry: stale telemetry-borne probe values are never consulted.
    char buf[diag::LINE_CHARS + 1];
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: SYNC CAND r13 s8");
    // The LAST line never keeps an old success: a plain empty window replaces it.
    st.on_window_analysis(0, -1, 0);
    CHECK(st.last() == LastResult::NoDecode);
    CHECK_FALSE(st.probe_valid());
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: NO DECODE");
}

TEST_CASE("a probe without a fresh debug message for the window is not shown") {
    DiagState st;
    // Window N produced a strong probe anchor and it is displayed.
    st.on_window_analysis(0, 100, 9);
    char buf[diag::LINE_CHARS + 1];
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: SYNC CAND r100 s9");
    // Window N+1: the UI found no fresh probe for that window (no debug
    // message arrived), so it forwards -1: no stale score may stay visible.
    st.on_window_analysis(0, -1, 0);
    CHECK(st.last() == LastResult::NoDecode);
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: NO DECODE");
    CHECK_FALSE(st.probe_valid());
}

TEST_CASE("confirm table: candidate storms must not block confirmation") {
    ConfirmTable ct;
    // Station A is heard once at t=0: stored as a grey candidate. Each
    // observe now carries the physical TX identity (start_br, col).
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 1000, 50));
    // 100 unrelated candidate/status events over the next 30 s: whatever the
    // message rate, they must NOT age A's slot (timers are monotonic time).
    // The events cycle through 10 distinct foreign stations so every observe
    // is a store or an in-place refresh of a foreign slot, never a confirm.
    // (A hundred DISTINCT stations cannot fit 12 slots at all; evicting the
    // oldest entry under slot-capacity pressure is a separate, documented
    // limitation - the bug fixed here was aging slots by MESSAGE COUNT.)
    char cs[13];
    for (uint32_t i = 0; i < 100; i++) {
        std::snprintf(cs, sizeof(cs), "ZZZ%02uXX", (unsigned)(i % 10));
        // Foreign stations repeating confirm among themselves - that is the
        // intended semantics; the storm's per-event traffic must simply not
        // touch A's slot timer (the pre-fix code aged every slot by count).
        // Their identities advance a slot period per repeat (352 block-rows),
        // as real transmissions do.
        ct.observe(cs, "JN49", 20, 1200 + (int16_t)((i % 10) * 8), i / 10,
                   2000 + i * 36, 40 + (int16_t)(i % 10));
    }
    CHECK(ct.used_slots() == 11);  // A plus the ten foreign stations
    // The same station repeats in a later window (t=120 s, a genuinely new
    // TX: a whole slot period past the stored identity): it MUST confirm,
    // despite the candidate storm in between.
    CHECK(ct.observe("A1ABC", "KO96", 33, 1502, 120, 1000 + 352, 50));
}

TEST_CASE("confirm table: independent repeats confirm without a time limit") {
    ConfirmTable ct;
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 1000, 50));
    // An independent repeat confirms.
    CHECK(ct.observe("A1ABC", "KO96", 33, 1499, (12u * 60u) - 60,
                     1000 + 352, 50));
    // LONG silence after confirmation: the flag is session-sticky, so a
    // later independent repeat STILL reports as confirmed - exactly like
    // the screen's green (which never greys on silence).
    CHECK(ct.observe("A1ABC", "KO96", 33, 1500,
                     (12u * 60u) - 60 + (12u * 60u) + 1,
                     1000 + 2 * 352, 50));
    // ...and the station is stored once (no slot multiplication).
    CHECK(ct.used_slots() == 1);
    // Even an unconfirmed base confirms after a day of silence.
    ConfirmTable ct2;
    CHECK_FALSE(ct2.observe("B2DEF", "LO10", 27, 1480, 0, 3000, 40));
    CHECK(ct2.observe("B2DEF", "LO10", 27, 1480,
                      (24u * 60u * 60u), 3000 + 352, 40));
    CHECK(ct2.observe("B2DEF", "LO10", 27, 1480,
                      (24u * 60u * 60u) + 120, 3000 + 2 * 352, 40));
    CHECK(ct2.used_slots() == 1);
}

TEST_CASE("confirm table: frequency is display-only; an independent repeat confirms across frequencies") {
    ConfirmTable ct;
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 1000, 50));
    // Same payload, a whole slot later, drifted 10 Hz: an INDEPENDENT
    // transmission - it confirms. The confirmation identity is the payload
    // (callsign/grid/dBm); the audio frequency never gates it, matching the
    // UI station table (a frequency change alone changes nothing).
    CHECK(ct.observe("A1ABC", "KO96", 33, 1510, 120, 1000 + 352, 50));
    CHECK(ct.used_slots() == 1);
    // A different station adds its own slot.
    CHECK_FALSE(ct.observe("B2DEF", "LO10", 27, 1480, 240, 2000, 60));
    CHECK(ct.used_slots() == 2);
    // reset() frees everything (the RESET button's new-session action).
    ct.reset();
    CHECK(ct.used_slots() == 0);
}

TEST_CASE("confirm table: same-onset column-separated re-decode never confirms") {
    ConfirmTable ct;
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 5000, 70));
    // One packet observed ~99 Hz apart within seconds: the same physical
    // onset (start_br within the +/-4 block-row tolerance) but a tone-0
    // column ~68 bins away. The metadata cannot prove two independent
    // transmissions, so this is uncertainty, not confirmation - and it does
    // not touch the slot (timer not re-armed, identity not adopted).
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1599, 5, 5003, 138));
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1599, 6, 4998, 139));
    CHECK(ct.used_slots() == 1);
    // Ambiguous observations do not confirm. A later independent repeat does.
    CHECK(ct.observe("A1ABC", "KO96", 33, 1500, (12u * 60u) + 1,
                     5000 + 352, 70));
    CHECK(ct.observe("A1ABC", "KO96", 33, 1500,
                     (12u * 60u) + 121, 5000 + 2 * 352, 70));
    CHECK(ct.used_slots() == 1);
}

TEST_CASE("confirm table: a re-delivery of the SAME TX never confirms") {
    ConfirmTable ct;
    // One physical transmission decoded once: a grey candidate.
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 5000, 70));
    // The SAME packet delivered again - same onset within the +/-4
    // block-row tolerance, column within the 6-bin tolerance, but arriving
    // with slightly different presentation (freq/agreement estimates from
    // another decode pass of the same TX): must NOT confirm and must not
    // touch the slot (single decode state).
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1501, 2, 5004, 72));
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1499, 3, 4996, 70));
    // S3: drift-trade hypotheses of ONE TX spread up to 4 columns at a br
    // gap of <=1 - a re-delivery on such a col hypothesis is still the same
    // physical TX, never a confirmation.
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 2, 5001, 74));
    CHECK(ct.used_slots() == 1);
    // The re-deliveries also never re-arm the acquisition window: still
    // only the t=0 anchor counts, so near the window end a same-TX
    // re-delivery stays unconfirmed - the slot persists (records live for
    // the session), so the status cannot silently flip through expiry.
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, (12u * 60u),
                           5002, 71));
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, (12u * 60u) + 1,
                           5002, 71));
    CHECK(ct.used_slots() == 1);
}

TEST_CASE("confirm table: a genuinely NEW TX of the same payload confirms") {
    ConfirmTable ct;
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 5000, 70));
    // A whole WSPR slot period (352 block-rows) later: beyond the same-TX
    // tolerance in onset, so this is another physical transmission of the
    // same payload - it confirms.
    CHECK(ct.observe("A1ABC", "KO96", 33, 1502, 120, 5000 + 352, 70));
    // ...and the slot's identity was adopted, so the next delivery of THAT
    // TX is again a non-confirming re-delivery.
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1502, 121, 5000 + 352 + 3, 71));
    // A third TX confirms again (repeatability across windows).
    CHECK(ct.observe("A1ABC", "KO96", 33, 1502, 240, 5000 + 2 * 352, 70));
    CHECK(ct.used_slots() == 1);
}

TEST_CASE("confirm table: locator/power change re-bases; one slot per callsign") {
    ConfirmTable ct;
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 33, 1500, 0, 5000, 70));
    // An INDEPENDENT TX of the same payload confirms...
    CHECK(ct.observe("A1ABC", "KO96", 33, 1500, 120, 5000 + 352, 70));
    // ...then the operator changes locator (or power): the SAME station
    // (same callsign) is re-based - the confirmation identity becomes the
    // new payload and the record un-confirms (it must be re-earned by an
    // independent repeat of the NEW payload). No parallel slot is opened;
    // nothing else about the station is erased.
    CHECK_FALSE(ct.observe("A1ABC", "JN49", 33, 1500, 240, 5000 + 2 * 352, 70));
    CHECK_FALSE(ct.observe("A1ABC", "KO96", 27, 1500, 360, 5000 + 3 * 352, 70));
    CHECK(ct.used_slots() == 1);
    // The re-based payload confirms again on its own independent repeat
    // within the acquisition window.
    CHECK(ct.observe("A1ABC", "KO96", 27, 1500, 480, 5000 + 4 * 352, 70));
}

TEST_CASE("ONE confirmation policy: screen table and log table agree on every step") {
    /* The shared scenario: the SAME decode sequence is fed through the
     * screen's stations::StationTable and the log's diag::ConfirmTable.
     * After every step the screen's green state (is_confirmed) must EQUAL
     * the log's observe() return - the screen and the normal SD log are
     * one policy, by construction and by test. Covers: first sight,
     * same-TX re-delivery, ambiguous same-onset re-decode (~99 Hz), the
     * confirming independent repeat, silence (sticky), locator/power
     * re-base, re-earn of the new payload, frequency drift, and a
     * duplicate delivery inside one window. */
    stations::StationTable st;
    ConfirmTable ct;
    auto step = [&](const char* cs, const char* grid, int8_t dbm, int16_t f,
                    uint32_t t, uint32_t br, int16_t col) {
        const bool unique = st.report(cs, grid, dbm, f, -10, 100, t, br, col, nullptr);
        const bool log_confirm = ct.observe(cs, grid, dbm, f, t, br, col);
        const auto* r = st.find(cs);
        /* A decode is a confirmed REPORT event exactly when the screen
         * accepted a unique report AND shows the station green. Duplicates
         * and ambiguous re-decodes are no event in either table. */
        const bool screen_event = unique && r && st.is_confirmed(*r);
        CHECK(log_confirm == screen_event);
        return log_confirm;
    };

    // First sight: grey on the screen, not a confirmed report in the log.
    CHECK_FALSE(step("A1ABC", "KO96", 33, 1500, 0, 5000, 70));
    // The SAME packet delivered again (same onset + column): nothing changes.
    CHECK_FALSE(step("A1ABC", "KO96", 33, 1501, 2, 5003, 72));
    // One packet re-decoded ~99 Hz apart within seconds (same onset,
    // column-separated): ambiguity, never confirmation.
    CHECK_FALSE(step("A1ABC", "KO96", 33, 1599, 5, 5002, 138));
    // An INDEPENDENT TX of the same payload within the acquisition window:
    // the screen turns green AND the log writes the report.
    CHECK(step("A1ABC", "KO96", 33, 1500, 120, 5000 + 352, 70));
    // Small frequency drift of the SAME payload, new onset: both stay/are
    // confirmed - the audio frequency is display-only in both tables.
    CHECK(step("A1ABC", "KO96", 33, 1512, 240, 5000 + 2 * 352, 71));
    // Silence past the acquisition window: the screen stays green and the
    // log still treats the next repeat as a confirmed report.
    CHECK(step("A1ABC", "KO96", 33, 1500, 1200, 5000 + 5 * 352, 70));
    // Locator change (operator moved): the record re-bases - the screen
    // greys immediately and the log records the decode as unconfirmed.
    CHECK_FALSE(step("A1ABC", "JN49", 33, 1500, 1320, 5000 + 6 * 352, 70));
    // Power change as well: still one station, still re-based/unconfirmed.
    CHECK_FALSE(step("A1ABC", "JN49", 27, 1500, 1440, 5000 + 7 * 352, 70));
    // The NEW payload re-earns confirmation on its own independent repeat.
    CHECK(step("A1ABC", "JN49", 27, 1500, 1560, 5000 + 8 * 352, 70));
    // A duplicate delivery of the same TX inside the window changes nothing.
    CHECK_FALSE(step("A1ABC", "JN49", 27, 1500, 1561, 5000 + 8 * 352 + 2, 71));
    // The next genuinely independent repeat is again a confirmed report.
    CHECK(step("A1ABC", "JN49", 27, 1500, 1680, 5000 + 9 * 352, 70));
}

TEST_CASE("status line staleness has a threshold and is monotonic") {
    CHECK_FALSE(diag::status_is_stale(0));
    CHECK_FALSE(diag::status_is_stale(diag::STATUS_STALE_TIMEOUT_S));
    CHECK(diag::status_is_stale(diag::STATUS_STALE_TIMEOUT_S + 1));
    CHECK(diag::status_is_stale(100000));
}

TEST_CASE("confirmed repeat of a later window shows CONFIRMED; single shows DECODED") {
    DiagState st;
    // Window 1: a lone decode, nothing confirmed earlier -> DECODED.
    st.on_window_analysis(1, -1, 0);
    CHECK(st.last() == LastResult::Decoded);
    // Re-processing analysis of the same window without a confirm flag does
    // not turn it into CONFIRMED.
    st.on_window_analysis(1, -1, 0);
    CHECK(st.last() == LastResult::Decoded);
    // Window 2: a repeat confirmed by the UI gate during THIS window.
    st.on_confirmed_decode();
    st.on_window_analysis(1, -1, 0);
    CHECK(st.last() == LastResult::Confirmed);
    char buf[diag::LINE_CHARS + 1];
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: CONFIRMED");
}

TEST_CASE("rephase counts down the skip and hides rows") {
    DiagState st;
    TelemetrySnapshot t = capture_snapshot(176, 1441792);
    t.phase = TelemetrySnapshot::PHASE_REPHASE;
    t.valid_flags = TelemetrySnapshot::VALID_SKIP;  // rows deliberately not valid
    t.skip_samples_remaining = 1048576;             // about 87 s at 12 kHz
    st.on_telemetry(t);

    char buf[diag::LINE_CHARS + 1];
    format_operation(st, 0, "REPHASE", buf);
    CHECK(std::string(buf) == "REPHASE 87s");

    // Countdown drains the skip.
    t.skip_samples_remaining = 24000;
    st.on_telemetry(t);
    format_operation(st, 0, "REPHASE", buf);
    CHECK(std::string(buf) == "REPHASE 2s");

    // Skip exhausted: back to capture, rows visible again.
    st.on_telemetry(capture_snapshot(3, 3 * 8192));
    format_operation(st, 0, "SCANNING", buf);
    CHECK(std::string(buf) == "SCANNING r3/351 FEC idle");
}

TEST_CASE("stale telemetry switches the operation line, last stays") {
    DiagState st;
    st.on_telemetry(capture_snapshot(10, 81920));
    st.on_window_analysis(0, -1, 0);

    CHECK(st.operation(DiagState::STALE_TIMEOUT_S) == Operation::Capture);
    CHECK(st.operation(DiagState::STALE_TIMEOUT_S + 1) == Operation::M4Stale);
    char buf[diag::LINE_CHARS + 1];
    format_operation(st, DiagState::STALE_TIMEOUT_S + 1, "SCANNING", buf);
    CHECK(std::string(buf).find("M4 STALE") != std::string::npos);
    // The last analysis verdict is independent of the live operation.
    CHECK(st.last() == LastResult::NoDecode);
    format_last(st, buf);
    CHECK(std::string(buf) == "LAST: NO DECODE");
}

TEST_CASE("decode elapsed and queue drops are reported") {
    DiagState st;
    TelemetrySnapshot t = capture_snapshot(176, 1441792, 3);
    t.phase = TelemetrySnapshot::PHASE_DECODE;
    t.valid_flags = TelemetrySnapshot::VALID_ROWS;
    st.on_telemetry(t);
    char buf[diag::LINE_CHARS + 1];
    format_operation(st, 0, "SYNCING", buf);
    // UI v1: the Decode phase shares the STATUS row, FEC busy is its state.
    CHECK(std::string(buf).rfind("SYNCING", 0) == 0);
    CHECK(std::string(buf).find("FEC busy") != std::string::npos);

    // The drop counter widens across 16-bit wrap without going backwards.
    TelemetrySnapshot d = t;
    d.telem_dropped = 65530;
    st.on_telemetry(d);
    CHECK(st.telem_dropped_total() == 65530);
    d.telem_dropped = 4;  // wrapped: 65536 + 4 seen, i.e. ten more drops
    st.on_telemetry(d);
    CHECK(st.telem_dropped_total() == 65540);
}

TEST_CASE("formatters never exceed the line budget") {
    DiagState st;
    TelemetrySnapshot t = capture_snapshot(176, 1441792, 255);
    t.valid_flags |= TelemetrySnapshot::VALID_SKIP | TelemetrySnapshot::VALID_DECODE_MS |
                     TelemetrySnapshot::VALID_PROBE;
    t.skip_samples_remaining = 0xFFFFFFFF;
    t.last_decode_ms = 65535;
    t.probe_row = 28;
    t.probe_score = 127;
    st.on_telemetry(t);
    st.on_window_analysis(0, 28, 127);
    st.on_telemetry(t);  // phase capture for the op line

    char buf[diag::LINE_CHARS + 1];
    format_operation(st, 0, "SCANNING", buf);
    format_last(st, buf);
    CHECK(std::strlen(buf) <= diag::LINE_CHARS);
}

/* ---- Candidate panel (candidate states only).
 * The UNCONFIRMED/CONFIRMED echo was removed - accepted reports are already
 * rendered by the station list ('?' marker, green confirmed rows). --------- */

TEST_CASE("candidate panel: NONE renders empty and CANDIDATE shows dots + age") {
    CandidateState p;
    char buf[diag::LINE_CHARS + 1];

    CHECK_FALSE(p.alive);
    format_candidate_panel(p, 0, buf);
    CHECK(std::string(buf) == "");  // blank panel, clearly NOT "..."

    p.observe(1493, 100);
    p.tick(100);
    CHECK(p.alive);
    format_candidate_panel(p, 100, buf);
    CHECK(std::string(buf) == "... 1493Hz 0s");

    // The age advances with the tick clock (arrival at 100 s).
    format_candidate_panel(p, 112, buf);
    CHECK(std::string(buf) == "... 1493Hz 12s");
}

TEST_CASE("candidate panel: CANDIDATE expires after 150 s, NONE differs from CANDIDATE") {
    CandidateState p;
    p.observe(1400, 1000);
    p.tick(1000);
    CHECK(p.alive);
    // Exactly TTL: still shown.
    p.tick(1000 + CandidateState::CANDIDATE_TTL_S);
    CHECK(p.alive);
    // One second past the TTL: expired, back to NONE (empty panel).
    p.tick(1000 + CandidateState::CANDIDATE_TTL_S + 1);
    CHECK_FALSE(p.alive);
    char buf[diag::LINE_CHARS + 1];
    format_candidate_panel(p, 1200, buf);
    CHECK(std::string(buf) == "");
    // A fresh observation resurrects the CANDIDATE state (a new candidate).
    p.observe(1550, 2000);
    p.tick(2005);
    CHECK(p.alive);
    format_candidate_panel(p, 2005, buf);
    CHECK(std::string(buf) == "... 1550Hz 5s");
}

TEST_CASE("candidate panel: repeated 1 Hz observation of the same candidate keeps its age") {
    /* The M4 re-publishes the pending observation every second. Only a NEW
     * candidate (different frequency) may re-arm the age; otherwise the
     * displayed age would be pinned at 0 s and the TTL would never expire. */
    CandidateState p;
    p.observe(1400, 1000);
    for (uint32_t t = 1001; t <= 1140; ++t)
        p.observe(1400, t);  // same frequency, every second
    char buf[diag::LINE_CHARS + 1];
    format_candidate_panel(p, 1140, buf);
    CHECK(std::string(buf) == "... 1400Hz 140s");
    // The TTL still fires on schedule.
    p.tick(1000 + CandidateState::CANDIDATE_TTL_S + 1);
    CHECK_FALSE(p.alive);
    // A genuinely different candidate re-arms it.
    p.observe(1550, 1200);
    format_candidate_panel(p, 1205, buf);
    CHECK(std::string(buf) == "... 1550Hz 5s");
}

TEST_CASE("candidate panel: freq 0 observation means no candidate and is ignored") {
    CandidateState p;
    // Contract: cand_freq_hz == 0 = no recent candidate; must not create a
    // placeholder just because a telemetry cycle elapsed.
    p.observe(0, 500);
    p.tick(501);
    CHECK_FALSE(p.alive);
}

TEST_CASE("candidate panel: CLEAR LIST and RX RESTART clear the candidate state") {
    CandidateState p;
    p.observe(1493, 100);
    p.tick(101);
    CHECK(p.alive);
    p.clear();  // CLEAR LIST
    CHECK_FALSE(p.alive);
    char buf[diag::LINE_CHARS + 1];
    format_candidate_panel(p, 101, buf);
    CHECK(std::string(buf) == "");
    // RX RESTART is the same clear().
    p.observe(1493, 200);
    p.tick(201);
    CHECK(p.alive);
    p.clear();
    CHECK_FALSE(p.alive);
}

TEST_CASE("candidate panel: updates never mutate the station table") {
    stations::StationTable table;
    CHECK(table.report("A1AAA", "JN49", 30, 1400, -5, 90, 10, 1000, 40, nullptr));
    CHECK(table.report("A1AAA", "JN49", 30, 1402, -5, 90, 130, 1000 + 352, 40,
                       nullptr));
    const stations::StationRecord* rec = table.find("A1AAA");
    REQUIRE(rec != nullptr);
    const size_t count_before = table.count();
    const uint16_t n_before = rec->n_reports;
    const bool confirmed_before = rec->confirmed;

    CandidateState p;
    // A storm of candidate observations across the 150 s TTL: none of them
    // may touch the table's count, N or confirmation state.
    for (uint32_t t = 0; t < 200; t++) {
        p.observe(1200 + (uint16_t)(t % 300), t);
        p.tick(t);
    }
    CHECK(table.count() == count_before);
    CHECK(table.find("A1AAA")->n_reports == n_before);
    CHECK(table.find("A1AAA")->confirmed == confirmed_before);
    CHECK(table.visible_count() == 1);
    CHECK(p.alive);
    // reset() of the table plus clear() leaves nothing behind.
    table.reset();
    p.clear();
    CHECK(table.count() == 0);
    CHECK_FALSE(p.alive);
}

TEST_CASE("candidate panel: formatter never exceeds the line budget") {
    CandidateState p;
    p.observe(1599, 0);
    char buf[diag::LINE_CHARS + 1];
    format_candidate_panel(p, CandidateState::CANDIDATE_TTL_S, buf);  // max age
    CHECK(std::strlen(buf) <= diag::LINE_CHARS);
    CHECK(std::string(buf) == "... 1599Hz 150s");
}

/* Diagnostics-correctness round 2026-11-19: the VALID_DIAG counters, their
 * interval deltas and the win-line write gate are host-testable again (see
 * the round-trip test cases appended below). */

/* ---- VALID_DIAG round trip + interval deltas + write gate (2026-11-19) ----
 *
 * The M4 publishes the cumulative stage counters in WSPRTelemetryMessage
 * guarded by VALID_DIAG (bit 8 of the 16-bit valid_flags word); the M0
 * copies them only while the bit is set, feeds IntervalDelta per win-line
 * write, and formats the SD win-line tail with append_diag_counters. The
 * tests below exercise that whole round trip through the same header the
 * device uses. */

using diag::append_diag_counters;
using diag::debug_line_due;
using diag::DiagCounters;
using diag::IntervalDelta;
using diag::TELEM_VALID_CAND;
using diag::TELEM_VALID_DIAG;
using diag::telemetry_has_diag;

namespace {

/* Model of WSPRTelemetryMessage's publish/consume of the VALID_DIAG block:
 * flags composed in a uint16_t exactly like publish_telemetry (proc_wspr_rx.cpp),
 * consumed through the same gate on_telemetry applies. */
struct TelemModel {
    uint16_t valid_flags{0};
    DiagCounters counters{};
};

TelemModel publish_diag(const DiagCounters& c, bool cand_live) {
    TelemModel m;
    m.valid_flags = (1 << 0) | (1 << 1);  // VALID_ROWS | VALID_CAPTURED
    m.valid_flags |= (1 << 5);            // VALID_LOSS
    if (cand_live) m.valid_flags |= TELEM_VALID_CAND;
    m.valid_flags |= TELEM_VALID_DIAG;
    m.counters = c;
    return m;
}

}  // namespace

TEST_CASE("VALID_DIAG round trip: candidate -> count -> published -> logged") {
    /* Bit 8 must survive a 16-bit flags word: the historical defect was an
     * 8-bit publish-side word that truncated it, leaving every M4 stage
     * counter reading zero despite successful reports. */
    static_assert(TELEM_VALID_DIAG == 0x100, "VALID_DIAG is bit 8");
    static_assert(TELEM_VALID_CAND == 0x80, "VALID_CAND is bit 7");
    static_assert(sizeof(uint16_t) * 8 > 8, "flags word must hold bit 8");

    DiagCounters m4{};
    /* Two candidates released; both entered the demod/aggregate stage; one
     * was rejected by association; one accepted REPORT on the M0. */
    m4.cand_selected = 2;
    m4.stage_entries = 2;
    m4.agg_reject = 1;

    TelemModel msg = publish_diag(m4, true);
    REQUIRE(telemetry_has_diag(msg.valid_flags));

    /* M0 side: the gated copy, then the win-line formatting. */
    DiagCounters m0{};
    if (telemetry_has_diag(msg.valid_flags)) m0 = msg.counters;
    CHECK(m0.cand_selected == 2);
    CHECK(m0.stage_entries == 2);
    CHECK(m0.agg_reject == 1);

    IntervalDelta dsel, dstg, drep;
    const uint32_t rep = 1;  // M0 counts its own accepted packet
    char buf[256];
    const size_t pos = append_diag_counters(buf, sizeof(buf), 0,
                                            m0.cand_selected, dsel.update(m0.cand_selected),
                                            m0.stage_entries, dstg.update(m0.stage_entries),
                                            rep, drep.update(rep),
                                            m0.snap_expired, m0.snap_overwritten,
                                            m0.agg_reject, m0.agg_fail, m0.agg_other);
    CHECK(std::string(buf).find(" sel=2 dSel=2") != std::string::npos);
    CHECK(std::string(buf).find(" stg=2 dStg=2") != std::string::npos);
    CHECK(std::string(buf).find(" rep=1 dRep=1") != std::string::npos);
    CHECK(std::string(buf).find(" rej=1 fail=0 oth=0") != std::string::npos);
    CHECK(std::string(buf).find("fA") == std::string::npos);  // old label gone
    (void)pos;
}

TEST_CASE("VALID_DIAG gate: a message without the bit never updates the counters") {
    DiagCounters stored{1, 1, 0, 0, 0, 0, 0};
    TelemModel msg = publish_diag(DiagCounters{}, false);
    msg.valid_flags &= (uint16_t)~TELEM_VALID_DIAG;  // simulate a truncated publish
    if (telemetry_has_diag(msg.valid_flags)) stored = msg.counters;
    /* The last known values survive untouched instead of showing zeros. */
    CHECK(stored.cand_selected == 1);
    CHECK(stored.stage_entries == 1);
}

TEST_CASE("interval deltas advance per logged write and never wrap on reset") {
    IntervalDelta d;
    /* First interval after startup: baseline 0, delta = session total. */
    CHECK(d.update(5) == 5);
    /* Normal advance. */
    CHECK(d.update(8) == 3);
    /* No change between writes: zero, not garbage. */
    CHECK(d.update(8) == 0);
    /* RX RESTART reset the cumulative counter (backwards step): the delta is
     * what accumulated since the reset, never the huge unsigned wrap a naive
     * subtraction of 0 - 9 would produce. */
    CHECK(d.update(2) == 2);
    CHECK(d.update(3) == 1);
    /* reset() re-baselines explicitly (UI restart handler). */
    d.reset();
    CHECK(d.update(4) == 4);
}

TEST_CASE("win-line write gate: quiet steady state is one write per minute") {
    constexpr uint32_t MIN = 60;  // seconds; statuses every 10 s
    uint32_t last_write = 0;
    unsigned writes = 0;
    /* 10 minutes of quiet band: heartbeat statuses every 10 s, no state
     * change, no decode. One write per minute; the first status does not
     * write on the interval branch alone (on the device the startup write
     * comes from the state-change branch, empty baseline vs the first word). */
    for (uint32_t t = 0; t <= 600; t += 10) {
        if (debug_line_due(false, false, t, last_write, MIN)) {
            writes++;
            last_write = t;
        }
    }
    CHECK(writes == 10);  // t=60, 120, ... 600
}

TEST_CASE("win-line write gate: state change and decode go out immediately") {
    CHECK(debug_line_due(true, false, 10, 5, 60));
    CHECK(debug_line_due(false, true, 10, 5, 60));
    /* Just below the interval: no write in the quiet steady state. */
    CHECK_FALSE(debug_line_due(false, false, 59, 0, 60));
    CHECK(debug_line_due(false, false, 60, 0, 60));
    /* Wrap-safe unsigned subtraction across the tick counter: 80 ticks
     * elapsed (5 - (-80) mod 2^32) -> due; 20 ticks -> not due. */
    CHECK(debug_line_due(false, false, 5, (uint32_t)-80, 60));
    CHECK_FALSE(debug_line_due(false, false, 5, (uint32_t)-20, 60));
}

TEST_CASE("diag tail formatter never exceeds its buffer") {
    char small[24];
    const size_t pos = append_diag_counters(small, sizeof(small), 0,
                                            123456, 1, 234567, 2, 345678, 3,
                                            456789, 567890, 678901, 789012, 890123);
    CHECK(pos < sizeof(small));
    CHECK(std::strlen(small) == pos);
    CHECK(small[sizeof(small) - 1] == '\0');
    /* Values that do not fit are truncated at the buffer edge, never
     * overwritten past it. */
    CHECK(std::string(small).rfind(" sel=123456", 0) == 0);
}
