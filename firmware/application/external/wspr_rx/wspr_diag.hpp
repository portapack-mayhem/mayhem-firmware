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

/* WSPR RX diagnostic helpers behind WSPR_RX_DIAG (the SD win line). No
 * ChibiOS, no widgets, no allocation: the same code runs on the device (M0)
 * and in the host unit tests. In T04 the screen state machine is gone; what
 * remains is exactly what the diagnostic build still uses:
 *   - IntervalDelta / debug_line_due (log rate budgeting),
 *   - LineWriter / WinLineCandidate / WinLineFields / format_win_line (the
 *     one bounded formatter for LOGS/WSPR_DEBUG.TXT).
 * The ordinary release build does not include this header at all. */

#ifndef __WSPR_DIAG_H__
#define __WSPR_DIAG_H__

#include <stdint.h>
#include <stddef.h>

namespace ui::external_app::wspr_rx {
namespace diag {

/* One screen line holds 30 characters of 8x16 font. The release STATUS line
 * (ui_wspr_rx.hpp) uses the same 30-character budget; kept here as the shared
 * documentation constant for the diagnostics code. */
static constexpr size_t LINE_CHARS = 30;

/* Per-interval delta of a cumulative counter between two SD win-line
 * writes. The documented logging interval is DEBUG_MIN_INTERVAL_S (60 s
 * quiet-band steady state, immediate when an analysis carried a decode), so
 * update() is called exactly once per actual log write and the returned value
 * is the counter's advance over that interval.
 *
 * A cumulative counter that went BACKWARDS means it was RESET (RX RESTART /
 * app restart zeroes the M4 counters while the baseline here keeps the old
 * value): re-baseline and report what accumulated since the reset, never the
 * huge unsigned wrap a naive subtraction would produce. The first interval
 * after startup baselines at zero, so its delta is simply the session total at
 * the first write - defined, not bogus. A genuine uint32 wrap of a live
 * counter is indistinguishable from a reset and reports the wrapped
 * remainder; acceptable for a diagnostics-only value. */
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

/* The SD win-line write gate: at most one entry per min_interval in the quiet
 * steady state, immediate when a decode landed. now_ticks/last_write_ticks are
 * on any monotonic unit as long as min_interval_ticks uses the same unit;
 * unsigned subtraction is wrap-safe. */
inline bool debug_line_due(bool state_changed, bool decoded, uint32_t now_ticks, uint32_t last_write_ticks, uint32_t min_interval_ticks) {
    return state_changed || decoded ||
           (now_ticks - last_write_ticks) >= min_interval_ticks;
}

/* ---- Bounded win-line writer (T01) ----
 *
 * The old on_status path chained std::snprintf calls and advanced `pos` by
 * each RETURN value. That value is the WOULD-BE length, so one truncated
 * field pushed pos to/past sizeof(line); the following `sizeof(line) - pos`
 * then underflowed (size_t) and `line + pos` pointed outside the buffer.
 * A separate snprintf for the " exp=..." block additionally never advanced
 * pos at all, so the counter tail overwrote that block. Both defects came
 * from writing without owning the position.
 *
 * LineWriter owns it: every append first clamps pos to cap-1, always leaves
 * the buffer N-terminated, and records `truncated` when a field did not fit
 * completely. It never touches bytes outside [0, cap). Adding the whole win
 * line through ONE formatter is also what makes the worst case provable in a
 * host test (see format_win_line below).
 *
 * cap == 0 is legal (nothing is written, truncated() becomes true); a null
 * buffer with cap > 0 is a caller bug and not defended against - the caller
 * passes a fixed array. */
class LineWriter {
   public:
    LineWriter(char* buf, size_t cap)
        : buf_{buf}, cap_{cap} {
        if (cap_ > 0)
            buf_[0] = '\0';
    }

    void append_str(const char* s) {
        if (cap_ == 0) {
            truncated_ = true;
            return;
        }
        if (s == nullptr)
            return;
        if (pos_ > cap_ - 1)
            pos_ = cap_ - 1;  // defensive clamp (the invariants below keep it < cap)
        while (*s != '\0') {
            /* Room for the character AND the terminator. Running out here is
             * the only way text is lost, and it is always reported. */
            if (pos_ + 1 >= cap_) {
                truncated_ = true;
                break;
            }
            buf_[pos_++] = *s++;
        }
        buf_[pos_] = '\0';
    }

    void append_u32(uint32_t v) {
        /* Reverse digit build, at most 10 digits (UINT32_MAX). No division by
         * a runtime divisor, no allocation; the temp is 10 bytes of stack in
         * a path that runs at most once per debug-line write. */
        char tmp[10];
        size_t n = 0;
        do {
            tmp[n++] = (char)('0' + (v % 10u));
            v /= 10u;
        } while (v != 0);
        char out[10];
        for (size_t i = 0; i < n; i++)
            out[i] = tmp[n - 1 - i];
        append_chars(out, n);
    }

    void append_i32(int32_t v) {
        if (v < 0) {
            append_chars("-", 1);
            /* Unsigned negation: well defined for INT32_MIN (its magnitude
             * 2147483648 does not fit int32), so no UB and no 64-bit math. */
            append_u32((uint32_t)0u - (uint32_t)v);
        } else {
            append_u32((uint32_t)v);
        }
    }

    size_t pos() const { return pos_; }
    bool truncated() const { return truncated_; }

   private:
    /* Append an explicit-length run (used by the digit formatters so they do
     * not need a terminator inside out[]). */
    void append_chars(const char* s, size_t n) {
        if (cap_ == 0) {
            truncated_ = true;
            return;
        }
        if (pos_ > cap_ - 1)
            pos_ = cap_ - 1;
        for (size_t i = 0; i < n; i++) {
            if (pos_ + 1 >= cap_) {
                truncated_ = true;
                break;
            }
            buf_[pos_++] = s[i];
        }
        buf_[pos_] = '\0';
    }

    char* buf_;
    size_t cap_;
    size_t pos_{0};
    bool truncated_{false};
};

/* The candidate slots of the win line. WSPRTelemetryMessage::max_candidates is
 * 4; wspr_diag.hpp deliberately does NOT include message.hpp (the host diag
 * test compiles this header alone), so the number is mirrored here and
 * ui_wspr_rx.cpp static_asserts the two agree. */
static constexpr size_t WINLINE_MAX_CANDIDATES = 4;

struct WinLineCandidate {
    int8_t sync_excess{0};
    int8_t row_offset{0};
    int16_t freq_hz{0};  // 0 marks an unused slot (a real decode is never at 0 Hz)
};

/* Every value the SD win line prints. Types follow WSPRTelemetryMessage so the
 * caller copies without narrowing surprises; the uint32 counters are the
 * cumulative VALID_DIAG values plus the M0-local report count and the
 * per-interval deltas.
 *
 * The field names printed by format_win_line are kept EXACTLY as before so
 * existing WSPR_DEBUG.TXT logs stay parseable, with the T01 additions
 * (snapW, snpB, fec, dFec, fpn, wait, dedup, ofl, pfr, pfa) in the documented
 * order. `last_skip` is still printed but T04 removed it from the message, so
 * the producer supplies 0. */
struct WinLineFields {
    const char* state_name{""};
    uint8_t decode_count{0};
    int16_t sync_score{0};
    uint8_t num_candidates{0};
    int16_t best_metric{0};
    int16_t last_skip{0};

    bool probe_valid{false};
    int8_t probe_row{-1};
    int8_t probe_score{0};

    WinLineCandidate candidates[WINLINE_MAX_CANDIDATES]{};

    /* Ring-scan health (VALID_LOSS). snap = snapshots dropped by the ring
     * overtaking the decode; snapW / snpB split the two refusal gates
     * (WSPR_SNAP_WEAK_SCORE / WSPR_SNAP_BAD_ARGS) that used to be lumped
     * into "overwritten". */
    uint32_t scan_expired{0};
    uint32_t snap_drops{0};
    uint32_t snap_weak{0};
    uint32_t snap_bad_args{0};
    uint32_t scan_max_lag{0};

    /* Receiver settings at write time. */
    uint32_t dial_hz{0};
    uint32_t lna{0};
    uint32_t vga{0};
    uint32_t amp{0};

    /* Cumulative (session) counters and their per-interval deltas. stg is
     * stage ENTRIES, fec is real stack_decode() calls, fpn is stack_decode()
     * no-path - three different things that must never be derived from one
     * another. */
    uint32_t cand_selected{0};
    uint32_t d_cand_selected{0};
    uint32_t stage_entries{0};
    uint32_t d_stage_entries{0};
    uint32_t fec_calls{0};
    uint32_t d_fec_calls{0};
    uint32_t fec_no_path{0};
    uint32_t reports{0};
    uint32_t d_reports{0};
    /* Reports the M4 ACCEPTED at the decoder (the integrated decode passed
     * every gate), before the shared queue. rep (the M0 packet count) can be
     * lower than acc by exactly the refused report pushes pfr, so report loss
     * becomes provable from one line instead of inferred. */
    uint32_t agg_reports{0};

    /* Aggregate dispositions (cumulative): code 1 is await_slot, NOT a
     * reject; ofl is the open-floor refusal; rej = slot-window+correlation;
     * fail = metric+agreement+unpack; oth = demod failure/unclassified. */
    uint32_t agg_await{0};
    uint32_t agg_dedup{0};
    uint32_t agg_open_floor{0};
    uint32_t agg_reject{0};
    uint32_t agg_fail{0};
    uint32_t agg_other{0};

    /* Refused shared-memory pushes: report packets vs status/debug (aux). */
    uint32_t push_fail_report{0};
    uint32_t push_fail_aux{0};
};

/* Format the complete SD win line into buf. Field order (unchanged names,
 * T01 additions in place):
 *
 *   win <state> decodes=<u> score=<d> cand=<u> metric=<d> skip=<d>
 *   [ probe=<d>/<d>]                       (when probe_valid)
 *   [ c<i>=<sync_excess>/<row_offset>/<freq_hz> ...]  (nonzero freq slots)
 *    exp=<u> snap=<u> snapW=<u> snpB=<u> lag=<u> dial=<u> lna=<u> vga=<u> amp=<u>
 *    sel=<u> dSel=<u> stg=<u> dStg=<u> fec=<u> dFec=<u> fpn=<u> rep=<u> dRep=<u>
 *    acc=<u> wait=<u> dedup=<u> ofl=<u> rej=<u> fail=<u> oth=<u> pfr=<u> pfa=<u>
 *
 * Returns the number of characters written (excluding the NUL). If any field
 * did not fit, `truncated` (when non-null) is set to true and the returned
 * text is a valid N-terminated prefix - the caller should append an explicit
 * marker rather than present it as complete. With cap >= 768 the documented
 * worst case (all uint32 fields UINT32_MAX, all int8/int16 fields at their
 * minima, 4 candidate slots, 10-digit dial, 9-char state) cannot truncate;
 * that is proved in the host test and in the T01 report. */
inline size_t format_win_line(char* buf, size_t cap, const WinLineFields& f, bool* truncated = nullptr) {
    LineWriter w{buf, cap};

    w.append_str("win ");
    w.append_str(f.state_name);
    w.append_str(" decodes=");
    w.append_u32(f.decode_count);
    w.append_str(" score=");
    w.append_i32(f.sync_score);
    w.append_str(" cand=");
    w.append_u32(f.num_candidates);
    w.append_str(" metric=");
    w.append_i32(f.best_metric);
    w.append_str(" skip=");
    w.append_i32(f.last_skip);

    if (f.probe_valid) {
        w.append_str(" probe=");
        w.append_i32(f.probe_row);
        w.append_str("/");
        w.append_i32(f.probe_score);
    }

    /* Only slots backed by a result are printed: an unused slot is
     * zero-filled and a real decode never sits at 0 Hz. */
    for (size_t i = 0; i < WINLINE_MAX_CANDIDATES; i++) {
        const WinLineCandidate& c = f.candidates[i];
        if (c.freq_hz == 0)
            continue;
        w.append_str(" c");
        w.append_u32((uint32_t)i);
        w.append_str("=");
        w.append_i32(c.sync_excess);
        w.append_str("/");
        w.append_i32(c.row_offset);
        w.append_str("/");
        w.append_i32(c.freq_hz);
    }

    w.append_str(" exp=");
    w.append_u32(f.scan_expired);
    w.append_str(" snap=");
    w.append_u32(f.snap_drops);
    w.append_str(" snapW=");
    w.append_u32(f.snap_weak);
    w.append_str(" snpB=");
    w.append_u32(f.snap_bad_args);
    w.append_str(" lag=");
    w.append_u32(f.scan_max_lag);
    w.append_str(" dial=");
    w.append_u32(f.dial_hz);
    w.append_str(" lna=");
    w.append_u32(f.lna);
    w.append_str(" vga=");
    w.append_u32(f.vga);
    w.append_str(" amp=");
    w.append_u32(f.amp);

    w.append_str(" sel=");
    w.append_u32(f.cand_selected);
    w.append_str(" dSel=");
    w.append_u32(f.d_cand_selected);
    w.append_str(" stg=");
    w.append_u32(f.stage_entries);
    w.append_str(" dStg=");
    w.append_u32(f.d_stage_entries);
    w.append_str(" fec=");
    w.append_u32(f.fec_calls);
    w.append_str(" dFec=");
    w.append_u32(f.d_fec_calls);
    w.append_str(" fpn=");
    w.append_u32(f.fec_no_path);
    w.append_str(" rep=");
    w.append_u32(f.reports);
    w.append_str(" dRep=");
    w.append_u32(f.d_reports);
    /* acc = reports the M4 accepted at the decoder; rep (M0) + pfr should
     * account for it, which is the report-loss proof. */
    w.append_str(" acc=");
    w.append_u32(f.agg_reports);

    w.append_str(" wait=");
    w.append_u32(f.agg_await);
    w.append_str(" dedup=");
    w.append_u32(f.agg_dedup);
    w.append_str(" ofl=");
    w.append_u32(f.agg_open_floor);
    w.append_str(" rej=");
    w.append_u32(f.agg_reject);
    w.append_str(" fail=");
    w.append_u32(f.agg_fail);
    w.append_str(" oth=");
    w.append_u32(f.agg_other);
    w.append_str(" pfr=");
    w.append_u32(f.push_fail_report);
    w.append_str(" pfa=");
    w.append_u32(f.push_fail_aux);

    if (truncated != nullptr)
        *truncated = w.truncated();
    return w.pos();
}

}  // namespace diag
}  // namespace ui::external_app::wspr_rx

#endif  // __WSPR_DIAG_H__
