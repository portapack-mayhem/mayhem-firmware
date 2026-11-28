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

// Station history for the standard RecentEntries widget. Each callsign has one
// row. Re-decodes near the last onset in the same capture leave the row alone.
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
/* Confirmation matches callsign, grid and transmitted power on the same dial. */
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

    char callsign[WSPR_CALL_MAX + 1]{};

    /* Latest accepted REPORT payload (updated by unique reports only). */
    char grid[5]{};
    int8_t dbm{0};
    int8_t snr_db{0};
    uint32_t last_report_s{0};

    /* Unique-REPORT counter, saturates at WSPR_N_MAX = 999. */
    uint16_t n_reports{0};

    // The capture generation and onset identify the latest accepted transmission.
    uint32_t session_id{0};
    uint32_t start_br{0};
    uint32_t dial_hz{0};

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

class StationTable {
   public:
    /* Storage consumed by ui::RecentEntriesTable: a std::list of records in
     * first-appearance order. */
    using Entries = std::list<StationRecord>;

    StationTable() = default;
    StationTable(const StationTable&) = delete;
    StationTable& operator=(const StationTable&) = delete;

    // Returns true when a distinct transmission updates the list.
    bool report(const char* callsign, const char* grid, int8_t dbm, int8_t snr_db,
                uint32_t now_s, uint32_t start_br, uint32_t session_id,
                uint32_t dial_hz, bool early_report) {
        char normalized_grid[sizeof(StationRecord::grid)];
        copy_bounded(normalized_grid, grid, sizeof(normalized_grid));
        auto found = find_entry(callsign);
        if (found != entries_.end()) {
            StationRecord& r = *found;
            if (same_onset(r, session_id, start_br))
                return false;

            /* A genuinely new transmission: unique REPORT. The node stays in
             * place, so first-appearance order is preserved. */
            if (r.n_reports < WSPR_N_MAX)
                r.n_reports++;
            /* else: documented saturation at 999. */

            // An independent repeat confirms the previous payload on the
            // same dial. A changed payload or dial starts confirmation over.
            r.confirmed = r.dial_hz == dial_hz &&
                          bounded_equal(r.grid, normalized_grid) &&
                          r.dbm == dbm;

            copy_bounded(r.grid, normalized_grid, sizeof(r.grid));
            r.dbm = dbm;
            r.snr_db = snr_db;
            r.last_report_s = now_s;
            r.session_id = session_id;
            r.start_br = start_br;
            r.dial_hz = dial_hz;
            r.early_report = early_report;

            return true;
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
        copy_bounded(r.callsign, callsign, sizeof(r.callsign));
        copy_bounded(r.grid, normalized_grid, sizeof(r.grid));
        r.dbm = dbm;
        r.snr_db = snr_db;
        r.last_report_s = now_s;
        r.n_reports = 1;
        r.session_id = session_id;
        r.start_br = start_br;
        r.dial_hz = dial_hz;
        r.early_report = early_report;
        return true;
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
                return bounded_equal(r.callsign, key);
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
};

}  // namespace stations
}  // namespace ui::external_app::wspr_rx

#endif  // __UI_STATION_TABLE_H__
