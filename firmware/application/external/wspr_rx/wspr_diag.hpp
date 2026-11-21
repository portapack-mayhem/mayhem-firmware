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

/* WSPR_RX_DIAG: pure state machine + fixed-buffer formatters behind the WSPR RX
 * diagnostic screen lines. No ChibiOS, no widgets, no allocation: the same code
 * runs on the device (M0) and in the host unit tests
 * (firmware/test/baseband/wspr_diag_test.cpp). Removal later means deleting
 * this file, the WSPR_RX_DIAG blocks in ui_wspr_rx.{hpp,cpp} and
 * proc_wspr_rx.{hpp,cpp}, and the diag host test. */

#ifndef __WSPR_DIAG_H__
#define __WSPR_DIAG_H__

#include <stdint.h>
#include <stddef.h>
#include <cstdio>

namespace ui::external_app::wspr_rx {
namespace diag {

/* One screen line holds 30 characters of 8x16 font. */
static constexpr size_t LINE_CHARS = 30;

/* The operation line: what the baseband is doing RIGHT NOW. WAIT M4 is shown
 * before the first telemetry message arrives, M4 STALE once telemetry stopped
 * arriving - a transport problem, not a verdict that the decoder died. */
enum class Operation : uint8_t {
    WaitM4 = 0,
    Capture = 1,
    Decode = 2,
    Rephase = 3,
    M4Stale = 4,
};

/* The LAST line: what the most recent window analysis concluded. Replaced on
 * every completed analysis - an old success is never kept on display. */
enum class LastResult : uint8_t {
    None = 0,  // no completed analysis yet; the line shows "LAST: --"
    NoDecode = 1,
    SyncCand = 2,
    Decoded = 3,
    Confirmed = 4,
};

/* Numeric snapshot of the WSPRTelemetryMessage, copied out under the UI
 * message-loop lock. Field semantics match the message; fields whose valid
 * bits are clear are ignored by the formatters. */
struct TelemetrySnapshot {
    uint8_t phase{0};
    /* Widened to 16 bits to match WSPRTelemetryMessage::valid_flags
     * (VALID_CAND = bit 7, VALID_DIAG = bit 8). The candidate panel and
     * the SD diag line consume those blocks in ui_wspr_rx.cpp directly from
     * the message; the snapshot keeps the low bits the formatters use. */
    uint16_t valid_flags{0};
    uint16_t rows{0};
    uint32_t captured_samples{0};
    uint32_t skip_samples_remaining{0};
    uint16_t last_decode_ms{0};
    uint8_t window_count{0};
    /* Low 16 bits of the baseband's total queue-drop counter; the UI widens
     * the delta with wrap-safe subtraction. */
    uint16_t telem_dropped{0};
    int8_t probe_row{-1};
    int8_t probe_score{0};
    /* Ring scan health counters (VALID_LOSS): starts scanned, candidate
     * jobs expired before processing, worst scan lag in block-rows, and
     * snapshots dropped because the ring overtook the decode. */
    uint32_t scan_scanned{0};
    uint32_t scan_expired{0};
    uint32_t scan_max_lag{0};
    uint32_t scan_snap_drops{0};
    /* Decode-thread OWN-stack free bytes, running minimum since launch
     * (VALID_STACK) — the D2 hardware check's decode-WA highwater. Not the
     * shared m4_stack_usage slot, which tracks the main M4 thread. */
    uint16_t decode_stack_free{0};

    static constexpr uint8_t VALID_ROWS = 1 << 0;
    static constexpr uint8_t VALID_CAPTURED = 1 << 1;
    static constexpr uint8_t VALID_SKIP = 1 << 2;
    static constexpr uint8_t VALID_DECODE_MS = 1 << 3;
    static constexpr uint8_t VALID_PROBE = 1 << 4;
    static constexpr uint8_t VALID_LOSS = 1 << 5;
    static constexpr uint8_t VALID_STACK = 1 << 6;

    /* Mirror of WSPRTelemetryMessage::Phase. */
    static constexpr uint8_t PHASE_CAPTURE = 1;
    static constexpr uint8_t PHASE_DECODE = 2;
    static constexpr uint8_t PHASE_REPHASE = 3;
};

/* ---- VALID_DIAG round trip (M4 counters -> telemetry -> M0 -> SD log) ----
 *
 * The M4 publishes cumulative stage counters in WSPRTelemetryMessage guarded
 * by VALID_DIAG = bit 8 of the 16-bit valid_flags word; the M0 copies them
 * only while that bit is set, and the per-interval deltas plus the SD
 * win-line tail are produced from them. These mirrors of the message's flag
 * bits and the helpers below are the host-tested half of that round trip
 * (wspr_diag_test.cpp); the M4 half is publish_telemetry in proc_wspr_rx.cpp,
 * whose flags word MUST be 16-bit (an 8-bit word truncates bit 8 to zero and
 * the counters then read zero forever - the defect fixed 2026-11-19). */
static constexpr uint16_t TELEM_VALID_CAND = 1 << 7;
static constexpr uint16_t TELEM_VALID_DIAG = 1 << 8;

/* The VALID_DIAG counter block, in message field order. */
struct DiagCounters {
    uint32_t cand_selected{0};  // candidates released by the scan funnel
    uint32_t stage_entries{0};  // candidate jobs that entered the demod/aggregate stage
    uint32_t snap_expired{0};
    uint32_t snap_overwritten{0};
    uint32_t agg_reject{0};
    uint32_t agg_fail{0};
    uint32_t agg_other{0};
};

/* The M0 consumption gate, exactly what WSPRRxView::on_telemetry applies to
 * the message's d_* fields: only a message carrying VALID_DIAG updates the
 * stored counters, so a publishing defect (e.g. a truncated flags word)
 * leaves the last known values untouched instead of showing zeros. */
inline bool telemetry_has_diag(uint16_t valid_flags) {
    return (valid_flags & TELEM_VALID_DIAG) != 0;
}

/* Per-interval delta of a cumulative counter between two SD win-line
 * writes. The documented logging interval is DEBUG_MIN_INTERVAL_S (60 s
 * quiet-band steady state, immediate on state change/decode), so update()
 * is called exactly once per actual log write and the returned value is
 * the counter's advance over that interval.
 *
 * A cumulative counter that went BACKWARDS means it was RESET (RX RESTART /
 * app restart zeroes the M4 counters while the baseline here keeps the old
 * value): re-baseline and report what accumulated since the reset, never
 * the huge unsigned wrap a naive subtraction would produce. The first
 * interval after startup baselines at zero, so its delta is simply the
 * session total at the first write - defined, not bogus. A genuine uint32
 * wrap of a live counter is indistinguishable from a reset and reports the
 * wrapped remainder; acceptable for a diagnostics-only value. */
class IntervalDelta {
   public:
    uint32_t update(uint32_t now) {
        const uint32_t delta = (now >= prev_) ? (now - prev_) : now;
        prev_ = now;
        return delta;
    }

    /* Explicit re-baseline (RX RESTART clears the UI-side counters too). */
    void reset() { prev_ = 0; }

   private:
    uint32_t prev_{0};
};

/* The SD win-line write gate (UI_CANDIDATE_DEPLOYMENT_RESULT.md 2.2): at
 * most one entry per min_interval in the quiet steady state, immediate on a
 * state-word change or when a decode landed. now_ticks/last_write_ticks are
 * on any monotonic unit as long as min_interval_ticks uses the same unit;
 * unsigned subtraction is wrap-safe. */
inline bool debug_line_due(bool state_changed, bool decoded, uint32_t now_ticks, uint32_t last_write_ticks, uint32_t min_interval_ticks) {
    return state_changed || decoded ||
           (now_ticks - last_write_ticks) >= min_interval_ticks;
}

/* Fixed-buffer formatters. Every function writes into a caller buffer,
 * always N-terminates, never allocates. Declared here (before the candidate
 * trace helpers and the win-line tail) because both consume them. */
namespace detail {

inline size_t append_u32(char* p, size_t cap, size_t pos, uint32_t v) {
    char digits[10];
    size_t n = 0;
    do {
        digits[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v && n < sizeof(digits));
    while (n > 0 && pos + 1 < cap)
        p[pos++] = digits[--n];
    if (pos < cap)
        p[pos] = '\0';
    return pos;
}

inline size_t append_str(char* p, size_t cap, size_t pos, const char* s) {
    while (*s && pos + 1 < cap)
        p[pos++] = *s++;
    if (pos < cap)
        p[pos] = '\0';
    return pos;
}

}  // namespace detail

/* Candidate-level trace (WSPR_CANDIDATE_TRACE) is serialized as a "cand "
 * line directly in ui_wspr_rx.cpp on_status (a single std::snprintf into a
 * fixed buffer, reusing the win line's write path). Field layout:
 *   cand freq_hz,snap_outcome,agg_code
 * Enum fields are RAW numeric codes (255 = unknown / not-attempted); names
 * live only in the host-side test parser. */

class DiagState {
   public:
    /* Telemetry publishes at 1 Hz; this many seconds without one means the
     * M4->M0 message transport is not flowing. */
    static constexpr uint32_t STALE_TIMEOUT_S = 10;
    /* A sync candidate is announced only for a genuine probe excess: this
     * mirrors the decoder's own WSPR_PROBE_MIN_EXCESS acceptance threshold
     * (wspr_decode.c), so what the screen calls SYNC CAND is the same anchor
     * the tracker would steer on, not a stale or arbitrary score. */
    static constexpr int8_t SYNC_CAND_MIN_SCORE = 4;

    /* Copy the numeric fields out of an arriving telemetry message. Called on
     * the UI thread from the message handler; no shared state beyond this
     * object. */
    void on_telemetry(const TelemetrySnapshot& snapshot) {
        telem_ = snapshot;
        telem_seen_ = true;
    }

    /* The baseband finished one window analysis. num_results is the accepted
     * decode count; probe_row < 0 when the sync probe found nothing or when
     * the caller has no probe fresh for THIS window (the UI only forwards a
     * probe that arrived in the debug message immediately before this status
     * message - stale probe values from earlier windows are never shown).
     * The probe anchor shown by the LAST line comes from this call, not from
     * live telemetry: the baseband keeps the last probe forever once set, so
     * telemetry-borne values go stale. The per-window confirmation flag must
     * be cleared by the caller once the analysis result has been consumed
     * (see mark_window_consumed). */
    void on_window_analysis(uint8_t num_results, int8_t probe_row, int8_t probe_score) {
        has_analyzed_ = true;
        probe_valid_ = false;
        if (num_results > 0) {
            last_ = window_confirmed_ ? LastResult::Confirmed : LastResult::Decoded;
        } else if (probe_row >= 0 && probe_score >= SYNC_CAND_MIN_SCORE) {
            last_ = LastResult::SyncCand;
            probe_valid_ = true;
            probe_row_ = probe_row;
            probe_score_ = probe_score;
        } else {
            /* Deliberately reachable only after the first completed analysis:
             * on_window_analysis is that analysis, so NO DECODE can never be
             * shown for a window that was never analyzed. */
            last_ = LastResult::NoDecode;
        }
        window_confirmed_ = false;
    }

    /* A decode of the CURRENT window survived the confirmation gate (a repeat
     * of a station heard in an earlier window). Same-packet repeats of an
     * already-confirmed decode do not pass through here again uncounted: the
     * flag only feeds the LAST line of the window it was set in. */
    void on_confirmed_decode() {
        window_confirmed_ = true;
    }

    /* Operation for the given telemetry age in seconds (measured on the M0 UI
     * monotonic tick). If the UI thread hangs, the age stops growing too - a
     * known limitation, not a countermeasure. */
    Operation operation(uint32_t age_s) const {
        if (!telem_seen_)
            return Operation::WaitM4;
        if (age_s > STALE_TIMEOUT_S)
            return Operation::M4Stale;
        switch (telem_.phase) {
            case TelemetrySnapshot::PHASE_DECODE:
                return Operation::Decode;
            case TelemetrySnapshot::PHASE_REPHASE:
                return Operation::Rephase;
            case TelemetrySnapshot::PHASE_CAPTURE:
                return Operation::Capture;
            default:
                return Operation::WaitM4;
        }
    }

    LastResult last() const { return last_; }
    bool has_analyzed() const { return has_analyzed_; }
    /* The probe anchor of the LAST completed analysis, only valid between a
     * SYNC CAND verdict and the next analysis. probe_row is a low-8-bit hint
     * of the absolute start block-row the baseband forwards (a full 0..351
     * id does not fit the message field). */
    bool probe_valid() const { return probe_valid_; }
    int8_t probe_row() const { return probe_row_; }
    int8_t probe_score() const { return probe_score_; }
    const TelemetrySnapshot& telemetry() const { return telem_; }

    /* Total baseband-side telemetry drops seen so far, wrap-safe across the
     * 16-bit counter in the message. */
    uint32_t telem_dropped_total() {
        const uint16_t now = telem_.telem_dropped;
        const uint16_t delta = (uint16_t)(now - drops_seen_);
        drops_seen_ = now;
        drops_total_ += delta;
        return drops_total_;
    }

   private:
    TelemetrySnapshot telem_{};
    LastResult last_{LastResult::None};
    bool telem_seen_{false};
    bool has_analyzed_{false};
    bool window_confirmed_{false};
    bool probe_valid_{false};
    int8_t probe_row_{-1};
    int8_t probe_score_{0};
    uint16_t drops_seen_{0};
    uint32_t drops_total_{0};
};

/* The SD win-line diagnostic tail: everything from " sel=" to the unclassified
 * bucket. sel/stg/rep are cumulative since session start; dSel/dStg/dRep the
 * IntervalDelta values for this logging interval. Writes into buf at pos
 * (append helpers: bounded, always N-terminates, never allocates) and returns
 * the new pos. stg counts stage ENTRIES (snapshot-admitted candidate jobs),
 * not FEC calls and not failures. */
inline size_t append_diag_counters(char* buf, size_t cap, size_t pos, uint32_t sel, uint32_t dsel, uint32_t stage, uint32_t dstage, uint32_t rep, uint32_t drep, uint32_t snap_expired, uint32_t snap_overwritten, uint32_t agg_reject, uint32_t agg_fail, uint32_t agg_other) {
    struct Field {
        const char* name;
        uint32_t value;
    };
    const Field fields[] = {
        {" sel=", sel},
        {" dSel=", dsel},
        {" stg=", stage},
        {" dStg=", dstage},
        {" rep=", rep},
        {" dRep=", drep},
        {" snapE=", snap_expired},
        {" snapO=", snap_overwritten},
        {" rej=", agg_reject},
        {" fail=", agg_fail},
        {" oth=", agg_other},
    };
    for (const Field& f : fields) {
        pos = detail::append_str(buf, cap, pos, f.name);
        pos = detail::append_u32(buf, cap, pos, f.value);
    }
    return pos;
}

/* The operation line. decode_elapsed_s: seconds since the UI saw the Decode
 * phase start (used only when op == Operation::Decode). */
/* UI v1 STATUS line: "<signal word> <capture progress> FEC idle/busy".
 * state_word comes from the last status message (SCANNING/SYNCING/LOCKED/
 * LISTENING/TRACKING/NO STATUS Ns - the staleness override is composed by
 * the caller); it says WHAT the receiver is doing, the telemetry counters
 * add the transport/capture numbers. Debug counters are not rendered. */
inline void format_operation(const DiagState& st, uint32_t age_s, const char* state_word, char (&buf)[LINE_CHARS + 1]) {
    constexpr size_t cap = sizeof(buf);
    buf[0] = '\0';
    size_t pos = detail::append_str(buf, cap, 0, state_word);
    const TelemetrySnapshot& t = st.telemetry();

    if (st.operation(age_s) == Operation::WaitM4 ||
        st.operation(age_s) == Operation::M4Stale) {
        /* Telemetry transport itself is dead: show only its verdict. */
        detail::append_str(buf, cap, pos,
                           st.operation(age_s) == Operation::M4Stale ? " M4 STALE" : " WAIT M4");
        return;
    }

    if (st.operation(age_s) == Operation::Rephase) {
        /* Intentional dead air; rows are meaningless during the skip, the
         * remaining pause is the useful number. */
        if (t.valid_flags & TelemetrySnapshot::VALID_SKIP) {
            pos = detail::append_str(buf, cap, pos, " ");
            pos = detail::append_u32(buf, cap, pos, t.skip_samples_remaining / 12000u);
            pos = detail::append_str(buf, cap, pos, "s");
        }
        return;
    }

    /* Capture progress: block-rows 0..351 (352 br = one full ring =
     * 120.15 s), NOT the old 176-symbol window count. */
    if (t.valid_flags & TelemetrySnapshot::VALID_ROWS) {
        pos = detail::append_str(buf, cap, pos, " r");
        pos = detail::append_u32(buf, cap, pos, t.rows);
        pos = detail::append_str(buf, cap, pos, "/351");
    }
    /* FEC state: the busy/idle verdict comes from the telemetry phase;
     * the decode wall time only when its flag is valid. Busy capture
     * and FEC are simultaneous, legitimate states. */
    pos = detail::append_str(buf, cap, pos, " FEC ");
    if (t.phase == TelemetrySnapshot::PHASE_DECODE) {
        pos = detail::append_str(buf, cap, pos, "busy");
        if ((t.valid_flags & TelemetrySnapshot::VALID_DECODE_MS) &&
            t.last_decode_ms > 0) {
            pos = detail::append_str(buf, cap, pos, " ");
            pos = detail::append_u32(buf, cap, pos, t.last_decode_ms);
            pos = detail::append_str(buf, cap, pos, "ms");
        }
    } else {
        pos = detail::append_str(buf, cap, pos, "idle");
    }
}

/* The LAST line: the verdict of the most recent completed analysis only. */
inline void format_last(const DiagState& st, char (&buf)[LINE_CHARS + 1]) {
    constexpr size_t cap = sizeof(buf);
    buf[0] = '\0';
    size_t pos = detail::append_str(buf, cap, 0, "LAST: ");

    switch (st.last()) {
        case LastResult::None:
            detail::append_str(buf, cap, pos, "--");
            break;
        case LastResult::NoDecode:
            detail::append_str(buf, cap, pos, "NO DECODE");
            break;
        case LastResult::SyncCand:
            pos = detail::append_str(buf, cap, pos, "SYNC CAND");
            /* Numbers come from the analysis itself, never from live
             * telemetry: the baseband keeps its last probe valid forever, so
             * a telemetry-borne score could show an hours-old anchor. probe_
             * validity is cleared by every subsequent analysis, stale or
             * decoded. */
            if (st.probe_valid()) {
                pos = detail::append_str(buf, cap, pos, " r");
                pos = detail::append_u32(buf, cap, pos, (uint32_t)(int32_t)st.probe_row());
                pos = detail::append_str(buf, cap, pos, " s");
                pos = detail::append_u32(buf, cap, pos, (uint32_t)(int32_t)st.probe_score());
            }
            break;
        case LastResult::Decoded:
            detail::append_str(buf, cap, pos, "DECODED");
            break;
        case LastResult::Confirmed:
            detail::append_str(buf, cap, pos, "CONFIRMED");
            break;
    }
}

/* The metrics line: AGE of the last accepted telemetry, the last analysis
 * wall time, the window counter and the telemetry queue-drop count. */
/* The confirmation table: the LOG side of the ONE confirmation policy the
 * screen (stations::StationTable) and this table share. Both tables are
 * structurally the same decision procedure, so the screen's green '?'/' '
 * state and what the normal log treats as a report CANNOT drift apart:
 *
 *  - Key: CALLSIGN (one slot per callsign, SLOTS = WSPR_STATION_MAX = 12).
 *    A changed locator or power is the SAME station; it re-bases the
 *    confirmation identity and un-confirms the record - it never opens a
 *    parallel track and never erases the observation history.
 *  - Confirmation identity is the PAYLOAD (grid + dBm) anchored per
 *    callsign; the audio frequency is display-only - a frequency change
 *    alone never changes any state.
 *  - Independence is the physical onset only: start_br beyond +-4
 *    block-rows from the stored identity. Two decodes sharing an onset
 *    (same-TX re-delivery, or one packet re-decoded ~99 Hz apart under
 *    another column hypothesis) are ambiguity, never confirmation, and
 *    touch nothing.
 *  - Acquisition: any independent same-payload repeat confirms during this
 *    session, regardless of elapsed time. Once confirmed, silence does not
 *    clear confirmation. A changed grid/power re-bases the payload.
 *  - Records persist for the session; capacity eviction (oldest
 *    last-seen) is the only removal, mirroring the station table.
 *
 * Returns TRUE exactly when the screen would show (or keep) green: an
 * independent same-payload repeat that confirms, or any repeat of an
 * already-confirmed station. Everything else (first sight, ambiguous
 * re-delivery, payload re-base) returns false
 * and is logged as an unconfirmed decode. Time is MONOTONIC WALL TIME in
 * seconds supplied by the caller - NEVER a count of status/debug/candidate
 * messages. Pure logic, host-tested TOGETHER with StationTable on one
 * shared scenario (see wspr_diag_test.cpp). */
class ConfirmTable {
   public:
    /* No acquisition timeout: confirmation lasts for the session. */
    /* Same capacity as the screen table: stations::WSPR_STATION_MAX. */
    static constexpr size_t SLOTS = 12;
    /* Physical TX identity tolerance, mirroring the baseband's same-
     * transmission window and stations::STATION_TX_BR/COL_TOLERANCE:
     * onset within +-4 block-rows and tone-0 column within +-6 bins.
     * The col tolerance is deliberately wider than the baseband's own
     * same-station match (WSPR_AGG_COL_MATCH = 2): the S3 study measured
     * drift-trade hypotheses of ONE TX spreading up to 4 columns at a br
     * gap of <=1, so a same-TX
     * re-delivery on another col hypothesis would otherwise be mistaken
     * for a new TX (false CONFIRMED). Distinct stations stay separated:
     * their col gap in the fixtures is >= 3 bins and usually >= 20, and
     * coinciding columns are still told apart by their spread-out br /
     * frequency. Columns never wrap in frequency, so a plain difference
     * is safe; start_br is an absolute monotonic ring id compared like the
     * baseband event dedup does, via a signed 32-bit difference. */
    static constexpr int32_t TX_BR_TOLERANCE = 4;
    static constexpr int32_t TX_COL_TOLERANCE = 6;

    struct Slot {
        bool used{false};
        char callsign[13]{};
        /* Confirmation base: the payload the next independent TX must
         * repeat, the monotonic time of the last matching observation, and the
         * session-sticky confirmation flag. Mirrors StationRecord's
         * confirm_* fields one for one. */
        bool base_valid{false};
        char grid[5]{};
        int8_t dbm{0};
        uint32_t anchor_s{0};
        bool confirmed{false};
        int16_t freq_hz{0};
        uint32_t last_seen_s{0};
        /* Identity of the physical transmission that last produced a
         * UNIQUE report for this slot. tx_known guards a default-
         * constructed slot against spuriously matching TX (0, 0). */
        bool tx_known{false};
        uint32_t start_br{0};
        int16_t col{0};
    };

    /* Feed one decode. The decision path below mirrors
     * stations::StationTable::report() step for step: same-TX re-delivery,
     * ambiguous same-onset re-decode, then the unique-report confirmation
     * logic (same payload -> acquire/refresh; different payload ->
     * re-base unconfirmed). */
    bool observe(const char* callsign, const char* grid, int8_t dbm, int16_t freq_hz, uint32_t now_s, uint32_t start_br, int16_t col) {
        Slot& s = slot_for(callsign);
        if (!s.used) {
            /* First sight: an unconfirmed candidate with a fresh
             * confirmation identity (mirrors the new-record path). */
            s.used = true;
            strlcpy_bounded(s.callsign, callsign, sizeof(s.callsign));
            s.freq_hz = freq_hz;
            s.last_seen_s = now_s;
            s.start_br = start_br;
            s.col = col;
            s.tx_known = true;
            rebase(s, grid, dbm, now_s);
            return false;
        }

        /* Same physical TX delivered again: never counts, never updates,
         * never confirms - keep everything exactly as it was. */
        if (s.tx_known && same_tx(s, start_br, col))
            return false;

        /* Same onset but a column outside the same-TX tolerance: the
         * same physical packet re-decoded under another column/frequency
         * hypothesis. Independence cannot be established from the
         * metadata, so this is uncertainty, not confirmation. */
        if (s.tx_known && same_onset(s, start_br))
            return false;

        /* A genuinely new transmission: unique REPORT (mirrors the
         * same-payload / re-base decision exactly). */
        const bool same_payload =
            s.base_valid &&
            strl_equal(s.grid, grid, sizeof(s.grid)) &&
            s.dbm == dbm;
        if (same_payload) {
            // Match the screen: any independent repeat in this session confirms.
            s.confirmed = true;
            s.anchor_s = now_s;
        } else {
            rebase(s, grid, dbm, now_s);
        }
        s.freq_hz = freq_hz;
        s.last_seen_s = now_s;
        s.start_br = start_br;
        s.col = col;
        s.tx_known = true;
        return s.confirmed;
    }

    /* New session: clear every slot element-wise. Slot is a POD, so a zeroed
     * slot is an unused slot; no assignment of the whole array (a C array is
     * not assignable) and no allocation. */
    void reset() {
        for (auto& s : slots_)
            s = Slot{};
    }

    size_t used_slots() const {
        size_t n = 0;
        for (const auto& s : slots_)
            if (s.used) n++;
        return n;
    }

   private:
    Slot slots_[SLOTS]{};

    /* Payload re-base (different grid/power): the identity is reset to an
     * unconfirmed candidate; the callsign's observation history is kept.
     * Mirrors StationTable's payload-change branch. */
    static void rebase(Slot& s, const char* grid, int8_t dbm, uint32_t now_s) {
        s.confirmed = false;
        strlcpy_bounded(s.grid, grid, sizeof(s.grid));
        s.dbm = dbm;
        s.anchor_s = now_s;
        s.base_valid = true;
    }

    Slot& slot_for(const char* callsign) {
        Slot* free_slot = nullptr;
        Slot* victim = &slots_[0];
        for (auto& s : slots_) {
            if (s.used && strl_equal(s.callsign, callsign, sizeof(s.callsign)))
                return s;
            if (!s.used && free_slot == nullptr)
                free_slot = &s;
            if (s.last_seen_s < victim->last_seen_s)
                victim = &s;
        }
        /* A free slot takes the record; only a FULL table evicts the least-
         * recently-seen entry - the same policy as the station table's
         * eviction (without a selection concept, which only exists on the
         * screen). */
        if (free_slot != nullptr) {
            *free_slot = Slot{};
            return *free_slot;
        }
        *victim = Slot{};
        return *victim;
    }

    /* Bounded equality for fixed-size always-terminated strings. */
    static bool strl_equal(const char* a, const char* b, size_t cap) {
        for (size_t i = 0; i < cap; i++) {
            if (a[i] != b[i]) return false;
            if (a[i] == '\0') return true;
        }
        return true;
    }

    /* Same physical transmission? True only when the slot already carries
     * an identity, the onset is inside the same-TX block-row tolerance AND
     * the tone-0 column is inside the same-TX column tolerance. A
     * column-separated decode at the same onset is the ambiguous case -
     * never a same-TX dedup and never a confirmation. */
    static bool same_tx(const Slot& s, uint32_t start_br, int16_t col) {
        if (!s.tx_known)
            return false;
        if (!same_onset(s, start_br))
            return false;
        const int32_t dcol = col > s.col ? (int32_t)(col - s.col)
                                         : (int32_t)(s.col - col);
        return dcol <= TX_COL_TOLERANCE;
    }

    /* Same physical onset (packet start within the block-row tolerance)?
     * True also for column-separated re-decodes of one packet. */
    static bool same_onset(const Slot& s, uint32_t start_br) {
        const int32_t dbr = (int32_t)(start_br - s.start_br);
        return dbr >= -TX_BR_TOLERANCE && dbr <= TX_BR_TOLERANCE;
    }

    static void strlcpy_bounded(char* dst, const char* src, size_t cap) {
        size_t i = 0;
        for (; i + 1 < cap && src[i] != '\0'; i++)
            dst[i] = src[i];
        dst[i] = '\0';
    }
};

/* Candidate panel state .
 *
 * One compact screen line showing ONLY the raw-candidate states:
 *   - NONE: no live candidate. The panel renders EMPTY, clearly distinct
 *     on screen from ...
 *   - CANDIDATE ("... <freq>Hz <age>s"): the M4 selected a search
 *     hypothesis (VALID_CAND with a non-zero audio frequency) and no
 *     accepted REPORT for it has arrived yet; ANY accepted REPORT answers
 *     the candidate and clears the state (the UI counts on_packet).
 * The UNCONFIRMED/CONFIRMED echo was REMOVED from the panel: accepted
 * reports are already rendered by the station list ('?' marker, green
 * confirmed rows), so the echo was duplicate visibility. The panel never
 * invents a callsign, never mutates the station table and never creates a
 * placeholder because a cycle elapsed.
 *
 * Expiry is OWNED by the UI (the M4 publishes raw observations without a
 * freshness window): a candidate dies CANDIDATE_TTL_S = 150 s after its
 * arrival tick (a bit more than one WSPR window), applied from the UI
 * per-second tick clock (the same monotonic seconds as everywhere else,
 * chTimeNow() / CH_FREQUENCY). CLEAR LIST and RX RESTART clear the state.
 *
 * Plain POD struct, no ctor body, no vtable, host-tested. */
struct CandidateState {
    static constexpr uint32_t CANDIDATE_TTL_S = 150;

    bool alive{false};
    uint16_t freq_hz{0};
    uint32_t arrival_s{0};

    /* A VALID_CAND observation arrived (the UI consumes ONLY cand_freq_hz).
     * freq_hz == 0 means "no recent candidate" and is ignored, so a telemetry
     * cycle can never create a placeholder on its own.
     *
     * The M4 re-publishes the same pending observation every second, so the
     * age is re-armed only for a GENUINELY NEW candidate: a different audio
     * frequency, or the first one after NONE/expiry. Re-arming on every
     * repeat would pin the age at 0 s and the TTL would never expire. */
    void observe(const uint16_t new_freq, const uint32_t arrival_tick) {
        if (new_freq == 0)
            return;
        if (alive && new_freq == freq_hz)
            return;  // repeat of the same still-pending observation
        alive = true;
        freq_hz = new_freq;
        arrival_s = arrival_tick;
    }

    /* Per-second tick: expires the candidate past the TTL. */
    void tick(const uint32_t now_s) {
        if (alive && (now_s - arrival_s) > CANDIDATE_TTL_S)
            alive = false;
    }

    /* CLEAR LIST / RX RESTART / an accepted REPORT: drop the state. */
    void clear() {
        alive = false;
        freq_hz = 0;
        arrival_s = 0;
    }
};

/* The candidate panel line. Fixed buffer, always N-terminated, never
 * allocates. NONE renders as an EMPTY string (the panel is blank - on screen
 * clearly different from the "..." of a candidate awaiting decode). */
inline void format_candidate_panel(const CandidateState& c, const uint32_t now_s, char (&buf)[LINE_CHARS + 1]) {
    constexpr size_t cap = sizeof(buf);
    buf[0] = '\0';
    if (!c.alive)
        return;
    const uint32_t age_s = (now_s >= c.arrival_s) ? (now_s - c.arrival_s) : 0;
    /* One snprintf: the append helpers cost more code at this call site than
     * the single format string they replace (m4_code budget). */
    std::snprintf(buf, cap, "... %uHz %us", (unsigned)c.freq_hz,
                  (unsigned)age_s);
}

/* The LOSS/METRICS screen formatters were removed in UI v1 (user decision:
 * no debug counters on screen). The loss counters still reach the SD win
 * log line (ui_wspr_rx.cpp on_status), which is the diagnostic channel. */

/* The main status line staleness guard: the baseband only pushes a status
 * message when its scan produced a candidate, so a long quiet band can leave
 * the line showing whatever the last message said. Once no status has
 * arrived for this many seconds the UI replaces the state text with an
 * explicit NO STATUS + AGE readout instead of leaving an old Searching or
 * Locked word on screen. Two full windows plus margin. */
static constexpr uint32_t STATUS_STALE_TIMEOUT_S = 300;

inline bool status_is_stale(uint32_t status_age_s) {
    return status_age_s > STATUS_STALE_TIMEOUT_S;
}

}  // namespace diag
}  // namespace ui::external_app::wspr_rx

#endif  // __WSPR_DIAG_H__
