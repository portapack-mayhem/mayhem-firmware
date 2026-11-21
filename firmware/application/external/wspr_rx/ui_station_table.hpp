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

/* WSPR RX UI v1 fixed station table model (UI_SPEC_APPROVED_V1.md section 4).
 *
 * One record per callsign, fixed capacity WSPR_STATION_MAX = 12, ZERO dynamic
 * allocation: plain fixed arrays, no vector, no string, no per-report heap
 * traffic. All time is MONOTONIC seconds supplied by the caller, so the model
 * is testable without ChibiOS (firmware/test/baseband/wspr_station_table_test.cpp).
 *
 * Semantics fixed by the spec:
 *  - N counts UNIQUE accepted REPORTs. Physical-TX identity dedup
 *    (start_br +-4 block-rows, tone-0 col +-6, the same tolerances as the
 *    diag ConfirmTable) is kept SEPARATE from the counter: a repeated
 *    delivery of the same TX never increments N and never touches the
 *    latest fields. N saturates at 999 (documented saturation, the row
 *    renders "999").
 *  - Order = first appearance this session; updates stay in place. Eviction
 *    at capacity removes the least-recently-updated record and NEVER the
 *    currently selected callsign (passed in on every insert).
 *  - Confirmation is a SESSION property keyed to the payload (callsign +
 *    grid + dBm; the audio frequency is display-only, so a frequency change
 *    alone never changes a station's status). An independent physical TX
 *    repeating the SAME payload at any time in the session acquires
 *    confirmation; once acquired, silence NEVER clears it - only a changed
 *    payload re-bases the record to unconfirmed. The record, its N and its
 *    confirmation therefore survive arbitrary quiet periods; freshness is a
 *    separate display metric (age_s()).
 *  - Independence is established from the physical onset ONLY. Two decodes
 *    sharing an onset (start_br within +/-STATION_TX_BR_TOLERANCE) are the
 *    same physical packet even when their tone-0 columns - and therefore
 *    audio frequencies - differ (one packet has been seen re-decoded ~99 Hz
 *    apart within seconds). Such an observation is recorded as an ambiguous
 *    observation (n_ambiguous): it does NOT increment N and does NOT
 *    confirm, because the available metadata cannot prove two independent
 *    transmissions. A genuinely new TX starts a whole 352-row slot period
 *    away, far outside the onset tolerance.
 *  - There is no visibility filter. Every accepted REPORT is visible;
 *    agreement_pct is a display metric only, never a correctness
 *    probability and never a reason to hide a row.
 *  - CLEAR LIST / reset() clears records, counters and confirmation state
 *    completely; the next report starts fresh.
 */

#ifndef __UI_STATION_TABLE_H__
#define __UI_STATION_TABLE_H__

#include <stddef.h>
#include <stdint.h>

namespace ui::external_app::wspr_rx {
namespace stations {

/* Fixed capacity, documented in the spec (section 4/5): 12 records, 5 visible. */
static constexpr size_t WSPR_STATION_MAX = 12;
/* Max callsign length of a WSPR extended compound call; storage is +1 NUL. */
static constexpr size_t WSPR_CALL_MAX = 12;
/* Documented saturation of the unique-REPORT counter. */
static constexpr uint16_t WSPR_N_MAX = 999;

/* Physical-TX identity tolerances, mirroring diag::ConfirmTable. */
static constexpr int32_t STATION_TX_BR_TOLERANCE = 4;
static constexpr int32_t STATION_TX_COL_TOLERANCE = 6;
/* Confirmation matches callsign, grid and transmitted power; frequency is display-only. */
/* Confirmation has no time limit; only independent matching packets count. */

struct StationRecord {
    bool used{false};
    char callsign[WSPR_CALL_MAX + 1]{};

    /* Latest accepted REPORT payload (updated by unique reports only). */
    char grid[5]{};
    int8_t dbm{0};
    int8_t snr_db{0};
    uint8_t agreement_pct{0};
    int16_t freq_hz{0};
    /* Monotonic seconds of the last unique REPORT; valid only when
     * last_valid is set (cleared by reset). */
    uint32_t last_report_s{0};
    bool last_valid{false};

    /* Unique-REPORT counter, saturates at WSPR_N_MAX = 999. */
    uint16_t n_reports{0};

    /* Ambiguous observations: same payload AND same physical onset
     * (start_br within tolerance) but a column outside the same-TX
     * tolerance - most plausibly a re-decode of one packet under another
     * frequency hypothesis. Not an observation of an independent
     * transmission: never counted in N, never confirming. Confirmation
     * evidence (independent repeats) and observation count (N) are
     * deliberately separate fields. */
    uint16_t n_ambiguous{0};

    /* Identity of the physical TX that produced the LATEST unique report;
     * the dedup key. tx_known guards a default-constructed record against
     * spuriously matching TX identity (0, 0). */
    bool tx_known{false};
    uint32_t start_br{0};
    int16_t col{0};

    /* Confirmation identity: the payload snapshot the next independent TX
     * must repeat to confirm, and the monotonic time of the last matching
     * observation (the latest matching observation). confirmed is
     * SESSION-persistent: only a payload change clears it. */
    bool confirmed{false};
    bool confirm_base_valid{false};
    char confirm_grid[5]{};
    int8_t confirm_dbm{0};
    uint32_t confirm_last_s{0};
};

/* Empty-state distinction:
 *  - NoReports "No reports yet" (table itself is empty)
 *  - Reports   normal list display (every accepted report is visible) */
enum class EmptyState : uint8_t {
    Reports = 0,
    NoReports = 1,
};

class StationTable {
   public:
    /* Feed one decoded REPORT. Returns true when this was a UNIQUE accepted
     * report (new physical TX identity): N was incremented and the latest
     * fields were refreshed. A re-delivery of the same TX returns false and
     * changes nothing.
     *
     * selected_callsign: the currently selected row's callsign (or null when
     * nothing is selected). It is only consulted for eviction protection
     * when a 13th distinct station arrives.
     *
     * now_s: monotonic seconds. start_br/col: the physical TX identity. */
    bool report(const char* callsign, const char* grid, int8_t dbm, int16_t freq_hz, int8_t snr_db, uint8_t agreement_pct, uint32_t now_s, uint32_t start_br, int16_t col, const char* selected_callsign) {
        const size_t idx = find_index(callsign);
        if (idx < count_) {
            StationRecord& r = records_[idx];
            /* Same physical TX delivered again: never counts, never updates,
             * never confirms - keep everything exactly as it was. */
            if (r.tx_known && same_tx(r, start_br, col))
                return false;

            /* Same onset but a column outside the same-TX tolerance: the
             * same physical packet re-decoded under another column/frequency
             * hypothesis. Independence cannot be established from the
             * metadata, so this is uncertainty, not confirmation: do not
             * count it in N, do not refresh any latest field, do not touch
             * the confirmation anchor - just record the ambiguity. */
            if (r.tx_known && same_onset(r, start_br)) {
                if (r.n_ambiguous < UINT16_MAX)
                    r.n_ambiguous++;
                return false;
            }

            /* A genuinely new transmission: unique REPORT. */
            if (r.n_reports < WSPR_N_MAX)
                r.n_reports++;
            /* else: documented saturation at 999. */

            copy_bounded(r.grid, grid, sizeof(r.grid));
            r.dbm = dbm;
            r.snr_db = snr_db;
            r.agreement_pct = agreement_pct;
            r.freq_hz = freq_hz;
            r.last_report_s = now_s;
            r.last_valid = true;
            r.start_br = start_br;
            r.col = col;
            r.tx_known = true;

            /* Confirmation per the ConfirmTable contract: an independent TX
             * repeating the SAME payload confirms; a payload change
             * (different grid/power) re-bases the confirmation identity as an
             * unconfirmed candidate and never inherits the old state.
             *
             * Once confirmed, the session confirmation is STICKY: a later
             * identical repeat refreshes the acquisition anchor but can
             * never clear the flag just because time passed. When the
             * record is not yet confirmed, any independent matching repeat
             * confirms it regardless of elapsed time. */
            const bool same_payload =
                r.confirm_base_valid &&
                bounded_equal(r.confirm_grid, r.grid) &&
                r.confirm_dbm == r.dbm;
            if (same_payload) {
                // A second independent packet confirms at any time this session.
                r.confirmed = true;
                r.confirm_last_s = now_s;
            } else {
                r.confirmed = false;
                copy_bounded(r.confirm_grid, r.grid, sizeof(r.confirm_grid));
                r.confirm_dbm = r.dbm;
                r.confirm_last_s = now_s;
                r.confirm_base_valid = true;
            }
            recompute_view();
            return true;
        }

        /* New station. Evict the least-recently-updated record when full,
         * never the currently selected callsign. Eviction shifts the
         * remaining records left so first-appearance order is preserved. */
        if (count_ >= WSPR_STATION_MAX) {
            size_t victim = WSPR_STATION_MAX;  // none
            for (size_t i = 0; i < count_; i++) {
                if (selected_callsign != nullptr && selected_callsign[0] != '\0' &&
                    bounded_equal(records_[i].callsign, selected_callsign))
                    continue;  // selection is protected
                if (victim == WSPR_STATION_MAX ||
                    records_[i].last_report_s < records_[victim].last_report_s)
                    victim = i;
            }
            if (victim == WSPR_STATION_MAX) {
                /* Can only happen if every record IS the selected callsign,
                 * which contradicts one-record-per-callsign; drop nothing. */
                return false;
            }
            for (size_t i = victim; i + 1 < count_; i++)
                records_[i] = records_[i + 1];
            records_[count_ - 1] = StationRecord{};
            count_--;
        }

        StationRecord& r = records_[count_++];
        r = StationRecord{};
        r.used = true;
        copy_bounded(r.callsign, callsign, sizeof(r.callsign));
        copy_bounded(r.grid, grid, sizeof(r.grid));
        r.dbm = dbm;
        r.snr_db = snr_db;
        r.agreement_pct = agreement_pct;
        r.freq_hz = freq_hz;
        r.last_report_s = now_s;
        r.last_valid = true;
        r.n_reports = 1;
        r.start_br = start_br;
        r.col = col;
        r.tx_known = true;
        /* First sight: an unconfirmed candidate with a fresh confirmation
         * identity. */
        copy_bounded(r.confirm_grid, r.grid, sizeof(r.confirm_grid));
        r.confirm_dbm = r.dbm;
        r.confirm_last_s = now_s;
        r.confirm_base_valid = true;
        r.confirmed = false;
        recompute_view();
        return true;
    }

    /* CLEAR LIST: records, counters, confirmation and session display state
     * are all cleared; nothing stale survives. */
    void reset() {
        for (size_t i = 0; i < WSPR_STATION_MAX; i++)
            records_[i] = StationRecord{};
        count_ = 0;
        view_count_ = 0;
    }

    /* Record count (one per callsign). */
    size_t count() const { return count_; }
    /* Record by position in first-appearance order; null when out of range. */
    const StationRecord* record(size_t i) const {
        return (i < count_) ? &records_[i] : nullptr;
    }
    const StationRecord* find(const char* callsign) const {
        const size_t idx = find_index(callsign);
        return (idx < count_) ? &records_[idx] : nullptr;
    }

    /* True when this payload was confirmed during the session. Purely a
     * record property: it does NOT depend on the current time, so silence
     * can never make a confirmed station look unconfirmed again. */
    bool is_confirmed(const StationRecord& r) const {
        return r.confirmed;
    }

    /* Alias kept for the render path: the display state is the session
     * confirmation state, with no TTL applied. */
    bool effective_confirmed(const StationRecord& r) const {
        return r.confirmed;
    }

    /* Effective confirmation of the k-th visible record. */
    bool visible_confirmed(size_t k) const {
        return (k < view_count_) ? effective_confirmed(records_[view_[k]])
                                 : false;
    }

    /* How long ago the record's latest unique REPORT arrived, in monotonic
     * seconds. A separate display property from confirmation: the age grows
     * forever while the confirmed flag stays put. */
    uint32_t age_s(const StationRecord& r, uint32_t now_s) const {
        if (!r.last_valid)
            return 0;
        return (now_s >= r.last_report_s) ? (now_s - r.last_report_s) : 0;
    }

    /* Every accepted REPORT is visible. The view order is first-appearance
     * order; nothing is filtered by agreement. */
    size_t visible_count() const { return view_count_; }
    /* k-th visible record; null when out of range. */
    const StationRecord* visible_record(size_t k) const {
        return (k < view_count_) ? &records_[view_[k]] : nullptr;
    }

    EmptyState empty_state() const {
        return (count_ == 0) ? EmptyState::NoReports : EmptyState::Reports;
    }

   private:
    StationRecord records_[WSPR_STATION_MAX]{};
    size_t count_{0};
    /* Indices into records_ of the visible subset, in display order.
     * Mutable: a derived display cache, recomputed by the const
     * recompute_view() display path too. */
    mutable size_t view_[WSPR_STATION_MAX]{};
    mutable size_t view_count_{0};

    void recompute_view() const {
        view_count_ = 0;
        for (size_t i = 0; i < count_; i++)
            view_[view_count_++] = i;
    }

    /* Bounded (max 13 chars) equality for fixed-size NUL-terminated keys. */
    static bool bounded_equal(const char* a, const char* b) {
        for (size_t i = 0; i < WSPR_CALL_MAX + 1; i++) {
            if (a[i] != b[i]) return false;
            if (a[i] == '\0') return true;
        }
        return true;
    }

    size_t find_index(const char* callsign) const {
        /* The stored keys are bounded to WSPR_CALL_MAX characters; bound the
         * search key the same way so a longer input still matches its
         * truncated record (one record per callsign, never two). */
        char key[WSPR_CALL_MAX + 1];
        copy_bounded(key, callsign, sizeof(key));
        for (size_t i = 0; i < count_; i++)
            if (records_[i].used && bounded_equal(records_[i].callsign, key))
                return i;
        return count_;
    }

    /* Same physical onset (packet start within the block-row tolerance)?
     * True also for column-separated re-decodes of one packet. */
    static bool same_onset(const StationRecord& r, uint32_t start_br) {
        const int32_t dbr = (int32_t)(start_br - r.start_br);
        return dbr >= -STATION_TX_BR_TOLERANCE && dbr <= STATION_TX_BR_TOLERANCE;
    }

    static bool same_tx(const StationRecord& r, uint32_t start_br, int16_t col) {
        if (!same_onset(r, start_br))
            return false;
        return freq_diff(col, r.col) <= STATION_TX_COL_TOLERANCE;
    }

    static int32_t freq_diff(int16_t a, int16_t b) {
        return a > b ? (int32_t)(a - b) : (int32_t)(b - a);
    }

    static void copy_bounded(char* dst, const char* src, size_t cap) {
        size_t i = 0;
        for (; i + 1 < cap && src[i] != '\0'; i++)
            dst[i] = src[i];
        dst[i] = '\0';
    }
};

}  // namespace stations
}  // namespace ui::external_app::wspr_rx

#endif  // __UI_STATION_TABLE_H__
