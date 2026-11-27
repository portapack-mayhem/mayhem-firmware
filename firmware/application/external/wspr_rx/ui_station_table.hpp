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

/* WSPR RX station table model (UI_SPEC_APPROVED_V1.md section 4; T02/T03).
 *
 * One record per callsign, capacity WSPR_STATION_MAX = 12. T03 changed the
 * STORAGE to `std::list<StationRecord>` (the same container the standard
 * ui::RecentEntriesTable consumes) so the UI can share Mayhem's standard
 * recent-entries Header + Table components. This introduces ONE heap node per
 * station - bounded to 12, allocated on the first appearance of a callsign,
 * freed on eviction / reset(); updates are in place and allocate nothing. The
 * T02 fixed array (records_[12]) is gone.
 *
 * All time is MONOTONIC seconds supplied by the caller, so the model is
 * testable without ChibiOS (firmware/test/baseband/wspr_station_table_test.cpp).
 *
 * StationTable is the ONLY authority for a station record, N, confirmation
 * and (through its explicit result) the optional report log. There is no
 * second table and no second policy.
 *
 * Semantics fixed by the spec (unchanged by T03):
 *  - N counts UNIQUE accepted REPORTs. Physical-TX identity dedup
 *    (start_br +-4 block-rows, tone-0 col +-6) is kept SEPARATE from the
 *    counter: a repeated delivery of the same TX never increments N and
 *    never touches the latest fields. N saturates at 999 (documented
 *    saturation, the row renders "999").
 *  - Order = first appearance this session; updates stay in place (the node
 *    is never moved). Eviction at capacity removes the least-recently-updated
 *    node and appends the new station at the back.
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
 *  - The capture session restarts start_br at 0. Physical identity is
 *    therefore (session_id, start_br, col); equal numeric ids from different
 *    sessions are different transmissions. Each record also stores the dial
 *    (dial_hz) it was received on, and confirmation does not carry across a
 *    dial change (an exact dial change, in-band or cross-band: evidence from a
 *    different reception frequency is not evidence for this one).
 *  - There is no visibility filter. Every accepted REPORT is visible;
 *    agreement_pct is a display metric only, never a correctness
 *    probability and never a reason to hide a row.
 *  - CLEAR LIST / reset() clears records, counters and confirmation state
 *    completely (frees every node); the next report starts fresh.
 */

#ifndef __UI_STATION_TABLE_H__
#define __UI_STATION_TABLE_H__

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <list>

namespace ui::external_app::wspr_rx {
namespace stations {

/* Fixed capacity, documented in the spec (section 4): 12 records. */
static constexpr size_t WSPR_STATION_MAX = 12;
/* Max callsign length of a WSPR extended compound call; storage is +1 NUL. */
static constexpr size_t WSPR_CALL_MAX = 12;
/* Documented saturation of the unique-REPORT counter. */
static constexpr uint16_t WSPR_N_MAX = 999;

/* Physical-TX identity tolerances (the only dedup policy in the app). */
static constexpr int32_t STATION_TX_BR_TOLERANCE = 4;
static constexpr int32_t STATION_TX_COL_TOLERANCE = 6;
/* Confirmation matches callsign, grid and transmitted power; frequency is display-only. */
/* Confirmation has no time limit; only independent matching packets count. */

/* Bounded (max WSPR_CALL_MAX chars) equality for fixed-size NUL-terminated
 * keys. No allocation, no unbounded strcmp. */
inline bool bounded_equal(const char* a, const char* b) {
    for (size_t i = 0; i < WSPR_CALL_MAX + 1; i++) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;
    }
    return true;
}

/* Bounded copy: always NUL-terminates within cap. */
inline void copy_bounded(char* dst, const char* src, size_t cap) {
    size_t i = 0;
    for (; i + 1 < cap && src[i] != '\0'; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

/* Bounded callsign key. This is the key type the standard
 * ui::RecentEntriesTable uses (Entry::Key / Entry::key() /
 * Entry::invalid_key): a plain value type with no dynamic storage, so the
 * table can keep one by value and the model can return one by value.
 * Empty text is the invalid key ("nothing selected"). */
struct StationKey {
    char text[WSPR_CALL_MAX + 1]{};

    constexpr bool operator==(const StationKey& other) const {
        for (size_t i = 0; i < WSPR_CALL_MAX + 1; i++) {
            if (text[i] != other.text[i]) return false;
            if (text[i] == '\0') return true;
        }
        return true;
    }
    constexpr bool operator!=(const StationKey& other) const {
        return !(*this == other);
    }
};

struct StationRecord {
    /* Key contract required by ui::RecentEntriesTable<StationTable::Entries>:
     * key() is the bounded callsign, invalid_key the empty selection. */
    using Key = StationKey;
    static constexpr Key invalid_key{};

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
     * spuriously matching TX identity (0, 0).
     *
     * A capture session restarts start_br at 0. Identity is therefore
     * (session_id, start_br, col): a report from a NEW session whose numeric
     * start_br matches a historical row is an independent transmission, never
     * a duplicate. dial_hz records the frequency provenance of the record so a
     * report from another band cannot inherit the old band's confirmation. */
    bool tx_known{false};
    uint32_t session_id{0};
    uint32_t start_br{0};
    uint32_t dial_hz{0};
    int16_t col{0};

    bool confirmed{false};
    bool early_report{false};

    char marker() const {
        if (confirmed)
            return ' ';
        return early_report ? '?' : '*';
    }

    /* Bounded callsign key (empty for a default-constructed record). */
    Key key() const {
        Key k;
        copy_bounded(k.text, callsign, sizeof(k.text));
        return k;
    }
};

/* Empty-state distinction:
 *  - NoReports "No reports yet" (table itself is empty)
 *  - Reports   normal list display (every accepted report is visible) */
enum class EmptyState : uint8_t {
    Reports = 0,
    NoReports = 1,
};

/* Explicit outcome of one StationTable::report() call. The UI consumes this
 * to decide what to refresh, what to log and whether the decode was newly
 * confirmed - it never re-derives the decision from a second table. */
enum class ReportResult : uint8_t {
    Duplicate = 0,          // same physical TX re-delivered: nothing changes
    Ambiguous = 1,          // same onset, column outside tolerance: n_ambiguous++
    UniqueUnconfirmed = 2,  // independent TX; N++; latest fields updated; not confirmed
    UniqueConfirmed = 3,    // independent TX; N++; latest fields updated; record confirmed
};

struct ReportOutcome {
    ReportResult result{ReportResult::Duplicate};
    bool newly_confirmed{false};  // true ONLY when this report established confirmation
};

class StationTable {
   public:
    /* Storage consumed by ui::RecentEntriesTable: a std::list of records in
     * first-appearance order. */
    using Entries = std::list<StationRecord>;

    StationTable() = default;
    StationTable(const StationTable&) = delete;
    StationTable& operator=(const StationTable&) = delete;

    /* Feed one decoded REPORT and return its explicit outcome.
     *
     *  - Duplicate: a re-delivery of the same physical TX. Nothing changes
     *    (no N, no latest fields, no freshness, no confirmation).
     *  - Ambiguous: same onset but a column outside the same-TX tolerance.
     *    Only n_ambiguous increments.
     *  - UniqueUnconfirmed / UniqueConfirmed: an independent TX (new onset).
     *    N increments (saturated at WSPR_N_MAX), the latest fields and
     *    freshness refresh, and confirmation is re-evaluated from the
     *    payload. newly_confirmed is true only on the report that
     *    established confirmation.
     *
     * now_s: monotonic seconds. start_br/col: the physical TX identity.
     * session_id: capture-session generation that produced the report. A new
     * session restarts start_br at 0, so the numeric identity is only compared
     * within one generation. dial_hz: receiver dial when the report was
     * accepted; it is part of the record's frequency provenance and re-bases
     * confirmation when the band changed. */
    ReportOutcome report(const char* callsign, const char* grid, int8_t dbm, int16_t freq_hz, int8_t snr_db, uint8_t agreement_pct, uint32_t now_s, uint32_t start_br, int16_t col, uint32_t session_id = 0, uint32_t dial_hz = 0, bool early_report = false) {
        char normalized_grid[sizeof(StationRecord::grid)];
        copy_bounded(normalized_grid, grid, sizeof(normalized_grid));
        auto found = find_entry(callsign);
        if (found != entries_.end()) {
            StationRecord& r = *found;
            /* Same physical TX delivered again: never counts, never updates,
             * never confirms - keep everything exactly as it was. */
            if (r.tx_known && same_tx(r, session_id, start_br, col))
                return ReportOutcome{ReportResult::Duplicate, false};

            /* Same onset but a column outside the same-TX tolerance: the
             * same physical packet re-decoded under another column/frequency
             * hypothesis. Independence cannot be established from the
             * metadata, so this is uncertainty, not confirmation: do not
             * count it in N, do not refresh any latest field, do not touch
             * the confirmation anchor - just record the ambiguity. */
            if (r.tx_known && same_onset(r, session_id, start_br)) {
                if (r.n_ambiguous < UINT16_MAX)
                    r.n_ambiguous++;
                return ReportOutcome{ReportResult::Ambiguous, false};
            }

            /* A genuinely new transmission: unique REPORT. The node stays in
             * place, so first-appearance order is preserved. */
            if (r.n_reports < WSPR_N_MAX)
                r.n_reports++;
            /* else: documented saturation at 999. */

            // An independent repeat confirms the previous payload on the
            // same dial. A changed payload or dial starts confirmation over.
            const bool previously_confirmed = r.confirmed;
            r.confirmed = r.dial_hz == dial_hz &&
                          bounded_equal(r.grid, normalized_grid) &&
                          r.dbm == dbm;

            copy_bounded(r.grid, normalized_grid, sizeof(r.grid));
            r.dbm = dbm;
            r.snr_db = snr_db;
            r.agreement_pct = agreement_pct;
            r.freq_hz = freq_hz;
            r.last_report_s = now_s;
            r.last_valid = true;
            r.session_id = session_id;
            r.start_br = start_br;
            r.col = col;
            r.dial_hz = dial_hz;
            r.tx_known = true;
            r.early_report = early_report;

            const bool newly_confirmed = r.confirmed && !previously_confirmed;
            return ReportOutcome{
                r.confirmed ? ReportResult::UniqueConfirmed
                            : ReportResult::UniqueUnconfirmed,
                newly_confirmed};
        }

        // Eviction preserves first-appearance order for the remaining rows.
        if (entries_.size() >= WSPR_STATION_MAX) {
            const auto victim = std::min_element(
                entries_.begin(), entries_.end(),
                [](const StationRecord& a, const StationRecord& b) {
                    return a.last_report_s < b.last_report_s;
                });
            entries_.erase(victim);
        }

        entries_.emplace_back();
        StationRecord& r = entries_.back();
        r.used = true;
        copy_bounded(r.callsign, callsign, sizeof(r.callsign));
        copy_bounded(r.grid, normalized_grid, sizeof(r.grid));
        r.dbm = dbm;
        r.snr_db = snr_db;
        r.agreement_pct = agreement_pct;
        r.freq_hz = freq_hz;
        r.last_report_s = now_s;
        r.last_valid = true;
        r.n_reports = 1;
        r.session_id = session_id;
        r.start_br = start_br;
        r.col = col;
        r.dial_hz = dial_hz;
        r.tx_known = true;
        r.early_report = early_report;
        return ReportOutcome{ReportResult::UniqueUnconfirmed, false};
    }

    /* CLEAR LIST: records, counters, confirmation and session display state
     * are all cleared (every node is freed); nothing stale survives. */
    void reset() {
        entries_.clear();
    }

    /* The container the standard ui::RecentEntriesTable draws. */
    Entries& entries() { return entries_; }
    const Entries& entries() const { return entries_; }

    /* Record count (one per callsign). */
    size_t count() const { return entries_.size(); }

    /* Record by position in first-appearance order; null when out of range.
     * Linear over the bounded list (max 12). */
    const StationRecord* record(size_t i) const {
        if (i >= entries_.size())
            return nullptr;
        auto it = entries_.begin();
        std::advance(it, (typename Entries::difference_type)i);
        return &*it;
    }

    const StationRecord* find(const char* callsign) const {
        const auto it = find_entry(callsign);
        return (it == entries_.end()) ? nullptr : &*it;
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

    /* Effective confirmation of the k-th visible record. Thin accessor: the
     * view IS the record order, no derived cache. */
    bool visible_confirmed(size_t k) const {
        const StationRecord* r = record(k);
        return r ? effective_confirmed(*r) : false;
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
     * order; nothing is filtered by agreement. These are thin compatibility
     * accessors over the record storage - there is no separate view cache. */
    size_t visible_count() const { return count(); }
    /* k-th visible record; null when out of range. */
    const StationRecord* visible_record(size_t k) const {
        return record(k);
    }

    EmptyState empty_state() const {
        return entries_.empty() ? EmptyState::NoReports : EmptyState::Reports;
    }

   private:
    Entries entries_{};

    /* Binary-compatible find over the bounded list. The stored keys are
     * bounded to WSPR_CALL_MAX characters; bound the search key the same way
     * so a longer input still matches its truncated record (one record per
     * callsign, never two). */
    Entries::iterator find_entry(const char* callsign) {
        char key[WSPR_CALL_MAX + 1];
        copy_bounded(key, callsign, sizeof(key));
        return std::find_if(
            entries_.begin(), entries_.end(),
            [&key](const StationRecord& r) {
                return r.used && bounded_equal(r.callsign, key);
            });
    }

    Entries::const_iterator find_entry(const char* callsign) const {
        char key[WSPR_CALL_MAX + 1];
        copy_bounded(key, callsign, sizeof(key));
        return std::find_if(
            entries_.begin(), entries_.end(),
            [&key](const StationRecord& r) {
                return r.used && bounded_equal(r.callsign, key);
            });
    }

    /* Same physical onset (packet start within the block-row tolerance)?
     * True also for column-separated re-decodes of one packet. Identity is
     * only comparable inside one capture session: every session restarts the
     * block-row counter at 0, so equal numeric ids from different generations
     * are different transmissions. */
    static bool same_onset(const StationRecord& r, uint32_t session_id, uint32_t start_br) {
        if (r.session_id != session_id)
            return false;
        const int32_t dbr = (int32_t)(start_br - r.start_br);
        return dbr >= -STATION_TX_BR_TOLERANCE && dbr <= STATION_TX_BR_TOLERANCE;
    }

    static bool same_tx(const StationRecord& r, uint32_t session_id, uint32_t start_br, int16_t col) {
        if (!same_onset(r, session_id, start_br))
            return false;
        return freq_diff(col, r.col) <= STATION_TX_COL_TOLERANCE;
    }

    static int32_t freq_diff(int16_t a, int16_t b) {
        return a > b ? (int32_t)(a - b) : (int32_t)(b - a);
    }
};

}  // namespace stations
}  // namespace ui::external_app::wspr_rx

#endif  // __UI_STATION_TABLE_H__
