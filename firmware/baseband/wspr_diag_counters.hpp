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

#ifndef _WSPR_DIAG_COUNTERS_HPP_
#define _WSPR_DIAG_COUNTERS_HPP_

#include <stdint.h>

/*
 * Slot-accumulator reason classification (T01 observability, contract section 4).
 *
 * wspr_agg_track_contribute() reports exactly one reason code per call through
 * wspr_agg_last_result_code() (see the enum in wspr_lib/wspr_decode.h). This
 * header is the ONE mapping from that code to the diagnostic buckets, shared by
 * the firmware decode thread (proc_wspr_rx.cpp) and the host tests, so both can
 * never disagree about what "await", "reject" or "fail" means.
 *
 * HONESTY RULE: code 1 (WSPR_AGG_RESULT_AWAIT_SLOT) is an ACCEPTED contribution
 * waiting for its next slot - it is NEVER a reject. The pre-T01 code counted it
 * into the reject bucket, which understated every integration in progress.
 *
 * fec_no_path (code 5) is kept in this struct so total() can account for every
 * classify() call, but it is deliberately NOT published as d_agg_fail: the real
 * FEC attempt is counted inside the decoder at the stack_decode() call site
 * (wspr_fec_calls()/wspr_fec_no_path()), and counting it here as well would
 * double count. The firmware publishes it from the decoder accessors only.
 *
 * Header-only, no allocation, no includes beyond <stdint.h>: safe to include
 * from the M4 firmware and from the host test suites.
 */
namespace wspr {
namespace diag_counters {

struct AggCounters {
    uint32_t await_slot;   // code 1: accepted contribution, count < MIN_COUNT
    uint32_t duplicate;    // code 2: same physical TX already contributed
    uint32_t slot_window;  // code 3: refused by the slot-window gate
    uint32_t correlation;  // code 4: refused by the message-consistency gate
    uint32_t open_floor;   // code 6: junk-grade candidate below the open floor
    uint32_t fec_no_path;  // code 5: no completed path (decoder counts the real one)
    uint32_t metric;       // code 7: completed path, metric below the report floor
    uint32_t agreement;    // code 9: completed path, re-encode agreement below min
    uint32_t unpack;       // code 10: completed path, unpack rejected the payload
    uint32_t report;       // code 8: integrated decode reported
    uint32_t other;        // code 0 and any unknown code
};

/*
 * Classify ONE wspr_agg_last_result_code() value into the buckets. The switch
 * is exhaustive over the published enum values; an unknown code (including the
 * receiver's 0xFF "never contributed" sentinel) lands in `other`. Code 1 maps to
 * await_slot and is never added to a reject bucket.
 */
inline void classify(uint8_t code, AggCounters& c) {
    switch (code) {
        case 0:  // WSPR_AGG_RESULT_NONE: no call / NULL argument
            c.other++;
            break;
        case 1:  // WSPR_AGG_RESULT_AWAIT_SLOT
            c.await_slot++;
            break;
        case 2:  // WSPR_AGG_RESULT_DUPLICATE
            c.duplicate++;
            break;
        case 3:  // WSPR_AGG_RESULT_SLOT_WINDOW
            c.slot_window++;
            break;
        case 4:  // WSPR_AGG_RESULT_CORRELATION
            c.correlation++;
            break;
        case 5:  // WSPR_AGG_RESULT_FEC_NO_PATH
            c.fec_no_path++;
            break;
        case 6:  // WSPR_AGG_RESULT_OPEN_FLOOR
            c.open_floor++;
            break;
        case 7:  // WSPR_AGG_RESULT_METRIC
            c.metric++;
            break;
        case 8:  // WSPR_AGG_RESULT_REPORT
            c.report++;
            break;
        case 9:  // WSPR_AGG_RESULT_AGREEMENT
            c.agreement++;
            break;
        case 10:  // WSPR_AGG_RESULT_UNPACK
            c.unpack++;
            break;
        default:
            c.other++;
            break;
    }
}

/* Session reset (RX RESTART): every bucket returns to zero. */
inline void reset(AggCounters& c) {
    c = AggCounters{};
}

/* Sum of every bucket: must equal the number of classify() calls made. */
inline uint32_t total(const AggCounters& c) {
    return c.await_slot + c.duplicate + c.slot_window + c.correlation +
           c.open_floor + c.fec_no_path + c.metric + c.agreement +
           c.unpack + c.report + c.other;
}

}  // namespace diag_counters
}  // namespace wspr

#endif  // _WSPR_DIAG_COUNTERS_HPP_
