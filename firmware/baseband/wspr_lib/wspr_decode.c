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

#include "wspr_decode.h"

#include "wspr_encode.h"
#include "wspr_unpack.h"

#include "wspr_fast_math.h"  // power_log2 (WSPR-local libm-free mantissa/exponent series)

#include <string.h>

/*
 * Decode pipeline:
 *
 * 1. Coarse sync scan over (time offset, tone-0 column) with the known sync
 *    bits: symbol n's tone sits at base + sync[n] + 2*data[n], so the sum of
 *    the better bin of the pair {base+sync[n], base+sync[n]+2} peaks sharply
 *    at the true alignment. No drift on this pass; it is a two-dimensional
 *    scan of only (num_rows - 162) * num_bins positions. The scan runs once
 *    per sub-row bank: each bank is a full view of the window on its own row
 *    grid (time_osr banks half a symbol apart cover every sub-row phase to
 *    within a quarter symbol).
 * 2. Drift refinement of the strongest coarse positions over a small linear
 *    drift grid.
 * 3. Soft demodulation: with the sync bit known, the data bit of symbol n is
 *    carried by the energy difference between the two bins of its pair.
 * 4. Stack sequential decoding of the K=32 rate-1/2 code. 2^31 Viterbi
 *    states are out of the question and no CRC guards the payload, so the
 *    decoder walks the best paths first under a Fano-style bias and keeps
 *    the best full path a bounded search can reach; the payload then has to
 *    survive wspr_unpack_message to be reported.
 * 5. Slot-aligned weight accumulation: every ring candidate's soft weights
 *    fold into the SLOT-ALIGNED track of its frequency column (a station
 *    repeats the same message across 2-minute slots, and each slot's weights
 *    are transmission-aligned in symbol order by the snapshot), and a track
 *    holding two or more slots decodes ONCE on its accumulated weights
 *    through the same gates - the sensitivity extension that reaches
 *    stations below the per-window threshold. Per-window decodes of the ring
 *    path are never reported: a single slot at low SNR decodes into
 *    unreliable payloads (impostors), so only the integrated decode is
 *    evidence.
 */

// 128 nodes x 10 bytes: store only the 50-bit payload, not the zero tail.
// The bounded frontier shares the 88 KiB M4 RAM with
// the dual-bank matrix, decoder stack, processor, and both DMA engines. A full
// pool evicts its weakest path; keep the weak-signal host cases when tuning it.
#define WSPR_STACK_NODES 128
#define WSPR_NODE_PATH_BYTES ((WSPR_PAYLOAD_BITS + 7) / 8)
#define WSPR_STACK_EFFORT 40000
#define WSPR_METRIC_LIMIT 250
// Crowded-band budget: eight stations in one real-ether recording out-compete
// each other for six slots, so the weaker true peaks never reach the decode.
#define WSPR_DRIFT_MIN (-3)
#define WSPR_DRIFT_MAX 3

// Require positive contrast against the opposite sync parity. Equal weighting
// removes the built-in noise/CW bias; weak candidates still need the zero-tail,
// unpack, path-metric and re-encoding gates before becoming reports.
#define WSPR_CANDIDATE_MIN_EXCESS 1

// Sequential decoding without a CRC completes a path on pure noise roughly
// one window in six (14% of random payloads unpack as structurally valid
// standard messages), so a completed path is not a report. A real
// transmission drives the path metric clearly positive; noise completions
// hover around zero. The gate sits between the two, calibrated on synthetic
// captures: real-ether calibration: three true stations survive at 26, impostors
// drop 15 -> 8 of 18 across the recording sweep; -24 dB synthetic decodes
// pass, some -26 dB seeds do not.
#define WSPR_REPORT_METRIC_MIN 20

// Re-encode agreement floor, in percent of the nonzero received symbols.
// A true decode regenerates channel symbols that match the received ones
// almost everywhere; a noise completion (no CRC to fail) disagrees on about
// half. Calibrated on the real-ether recording.
#define WSPR_MIN_AGREEMENT_PCT 62

typedef struct {
    int16_t metric;
    uint8_t depth;
    uint8_t path[WSPR_NODE_PATH_BYTES];  // only the 50 payload bits; tail is zero
} wspr_stack_node_t;

// The node pool and the decoded weights live in .bss: the decode thread's
// 4 KiB stack cannot hold them.
// Search nodes are dead once stack_decode returns. Reuse that storage for
// the SNR noise histogram, so the quality estimate costs no additional RAM.
static union {
    wspr_stack_node_t nodes[WSPR_STACK_NODES];
    uint16_t noise_histogram[64];
} search_workspace;
static int node_count;
static int8_t bit_weight[WSPR_CODED_BITS];
static int8_t sym_weight[WSPR_CODED_BITS];
// Best remaining metric gain per depth, for the branch-and-bound prune.
static int16_t suffix_gain[WSPR_SOURCE_BITS + 1];

/*
 * Slot-aligned weight accumulator - the sensitivity extension.
 *
 * A WSPR station repeats the SAME message in successive 2-minute slots
 * (typical duty 20-100%), and each slot's snapshot demodulation produces
 * sym_weight[] in TRANSMISSION-aligned symbol order (symbol 0 is the slot's
 * sync-anchored start). Summing N slots' weights adds the signal N times
 * while the independent noise grows only as sqrt(N), so the integrated
 * decode gains ~3 dB of SNR per doubling of N: a station each of whose slots
 * sits below the per-window acceptance gates can still clear them once
 * enough slots have been integrated.
 *
 * WHY slots and not payload keys: the per-window stack decode at -14 dB is
 * NOT reliable - the same signal decodes into DIFFERENT payloads in
 * different slots (noise draws independent impostor completions through the
 * no-CRC gates), so payload-keyed clusters integrated impostor weights and
 * evicted the true station. The track never looks at a per-window decode:
 * it integrates the slot's WEIGHTS keyed by FREQUENCY, and only the
 * integrated sum - with its ~3 dB/doubling advantage - is decoded and
 * reported. A single slot is never a report.
 *
 * Track identity is the tone-0 column: a candidate joins the track whose
 * anchor column is within +-2 bins (neighbouring bins are noise spread of
 * the same station); otherwise it opens a new track. The absolute start
 * block-row identifies the physical slot: a candidate within +-4 block-rows
 * of the track's last slot is a ghost anchor of the SAME transmission and
 * contributes nothing. Tracks age in SLOTS, not candidates: empty slots
 * leave the weights untouched (no contribution = no corruption) and a track
 * idle for more than WSPR_AGG_TRACK_MAX_AGE slots is reset (lazily, on the
 * next contribution to it, and first in line for eviction).
 *
 * A track decodes on the SUM of its accumulated weights (clipped into the
 * int8 the stack decoder reads), not the slot average - see the sum-vs-mean
 * note in the historical cluster design: the sum keeps every slot's full
 * magnitude, and the same gates separate harder on the sum scale, not
 * softer. After a SUCCESSFUL integrated decode the track resets: the next
 * slot starts a fresh integration cycle, so a station that changes its
 * message is never mixed with the old weights, and one message cannot be
 * reported twice off the same evidence.
 */
// WSPR_AGG_TRACKS comes from the header (the track-table geometry is public).
#define WSPR_AGG_MIN_COUNT 2      // slots accumulated before a decode is attempted
#define WSPR_AGG_TRACK_MAX_AGE 5  // idle slots before a track is dropped
#define WSPR_AGG_COL_MATCH 2      // same-station column tolerance, bins
// A new slot's start must land within this many block-rows of a whole WSPR
// slot period (352) after the track's last contribution. The sync-plateau of
// one transmission releases several start/drift hypotheses (with drift, a
// start shift trades against a drift shift), and those ghosts of the SAME TX
// must never count as slots.
#define WSPR_AGG_SLOT_TOL 20
// One 120 s WSPR slot is 1440000 samples = 351.5625 block-rows of the OSR2
// ring; 352 is the nominal spacing used to convert a block-row gap into slots.
#define WSPR_AGG_SLOT_ROWS 352
// Yield policy: after this many strong-grade (excess >= open floor)
// candidates rejected against a track - slot-window or message-consistency -
// from DISTINCT transmissions (hypotheses of one rejected TX charge at most
// once, see WSPR_AGG_REJECT_TX_TOL), the track resets and yields its table
// slot to the newcomer, with no weights crossing between the old and the new
// occupant. Two strong rejects from distinct transmissions are virtually
// never junk (the junk ceiling measured on the streaming suite is 9 against
// a floor of 10), so this bounds recovery from an old-payload or
// coincidental occupant to two strong slots plus one integration slot
// instead of the full idle TTL.
#define WSPR_AGG_TRACK_YIELD_REJECTS 2
// Rejected-TX identity for the yield counter. A strong candidate that no
// track can absorb records its onset (block-row + column, wide signed
// arithmetic) on the track; a further strong candidate within
// +-WSPR_AGG_REJECT_TX_TOL block-rows of that onset and within
// +-WSPR_AGG_REJECT_TX_COL_TOL columns of its column is a HYPOTHESIS OF THE
// SAME TRANSMISSION (the incremental scan releases one TX as several
// start/bank/drift jobs shifted by 1-2 block-rows) and charges the yield
// streak at most once - otherwise a single strong interference burst,
// surfaced as two scan hypotheses, would evict a healthy track. A genuinely
// new transmission starts a full slot later (>= ~300 block-rows), far
// outside this window. The col window is WIDER than the merge tolerance
// (WSPR_AGG_COL_MATCH): the host tolerance study measured that the
// drift-trade hypotheses of ONE strong TX spread up to 4 columns (tone-0
// 10 -> 6/14 at excess 11-37),
// so identity here must be looser than track matching. This dedup is per
// track and entirely separate from the accepted-weights event table
// (agg_events), which never sees rejected candidates.
#define WSPR_AGG_REJECT_TX_TOL 4
#define WSPR_AGG_REJECT_TX_COL_TOL 6
// Opening a NEW track is a commitment of one of the four table slots, and the
// incremental ring scan releases a FLOOD of junk-grade candidates (measured on
// the -14 dB host streaming suite: ~15 noise completions per 120 s slot at
// excess 5..8 against 2 real slots at excess 28+). A junk candidate that
// opens a track shadows the real station's slots (first-match column matching)
// and breaks integration, so OPENING requires more sync evidence than the
// scan gate: sync_score >= WSPR_AGG_OPEN_EXCESS_MIN (10: the measured junk ceiling on the -14 dB streaming suite is 9, while a -26 dB WSJT real slot still scores 10-13). MERGING into an existing
// track has no such floor - the slot-window and message-consistency gates
// already protect it - and a station that once anchored keeps integrating
// whatever its per-slot excess. KNOWN LIMITATION: a real transmission whose
// first slot scores below this floor never opens a track; the on-air aligned
// figure was calibrated at 6-7 excess, so marginal stations need the planned
// sensitivity study instead of a silent loss here.
#define WSPR_AGG_OPEN_EXCESS_MIN 10

typedef struct {
    int16_t weight[WSPR_CODED_BITS];  // accumulated soft weights, symbol order
                                      // (plain adds; T09-C1 invariant on the fold)
    uint32_t anchor_start_br;         // absolute start of the first slot
    uint32_t last_slot_br;            // absolute start of the newest slot
    int16_t col0;                     // anchor tone-0 column (track identity)
    int16_t last_sync_score;          // newest slot's drift-refined excess
    int8_t last_drift;                // newest slot's refined drift
    int8_t last_bank;                 // newest slot's bank parity
    uint8_t count;                    // slots accumulated (capped at
                                      // WSPR_AGG_MAX_COUNT, never wraps)
    uint8_t failed;                   // gate-failing integrated decodes
                                      // (budget WSPR_AGG_MAX_FAILED_DECODES)
    uint8_t rejected_strong;          // consecutive strong-grade candidates
                                      // rejected against this track (yield
                                      // policy, WSPR_AGG_TRACK_YIELD_REJECTS;
                                      // hypotheses of ONE rejected TX charge
                                      // at most once, see
                                      // WSPR_AGG_REJECT_TX_TOL)
    uint32_t last_reject_br;          // onset of the last strong rejected
                                      // candidate charged to the counter
                                      // (same-TX dedup identity)
    int16_t last_reject_col;          // its tone-0 column
    bool has_reject;                  // last_reject_* is valid
    uint8_t age;                      // idle slots since the last contribution
                                      // (derived from STREAM elapsed time)
    bool active;
#if defined(WSPR_RX_EARLY_REPORTS)
    bool early_valid;
    uint8_t early_source[WSPR_SOURCE_BYTES];
#endif
} wspr_agg_track_t;

static wspr_agg_track_t agg_tracks[WSPR_AGG_TRACKS];
#if defined(WSPR_RX_EARLY_REPORTS)
// One pending acknowledgement, owned by the serial decoder worker.
static wspr_agg_track_t* pending_early_track;

void wspr_agg_ack_early_report(void) {
    if (pending_early_track != NULL) {
        pending_early_track->early_valid = true;
        pending_early_track = NULL;
    }
}
#endif

// Column distance on the linear band axis (columns never wrap in frequency);
// defined with the ring readers further down, declared here for the dedup table.
static int ring_col_gap(int a, int b);

/*
 * Persistent EVENT dedup table (lives across candidate jobs, cleared only by
 * wspr_decode_reset_aggregation). Every ACCEPTED contribution records its packet
 * identity: absolute onset + tone-0 column. A later candidate of the SAME
 * transmission — the anchor family the sync plateau releases from other banks or
 * drift shifts (onset within +-WSPR_AGG_SLOT_TOL, column within the match
 * tolerance) — contributes NOTHING, even if the track was meanwhile reset by a
 * successful decode (without this table such a ghost would silently open a fresh
 * count-1 track). This is the formal replacement of the window decoder's
 * per-call agg_matched bitmask, which was local to one job and so let a
 * metric-rejected-but-payload-valid candidate weigh the same TX once per bank.
 */
#define WSPR_AGG_DEDUP_EVENTS 8
typedef struct {
    uint32_t start_br;
    int16_t col0;
    bool used;
} wspr_agg_event_t;
static wspr_agg_event_t agg_events[WSPR_AGG_DEDUP_EVENTS];
static unsigned agg_event_next = 0;

static bool agg_event_duplicate(uint32_t start_br, int16_t col) {
    for (int i = 0; i < WSPR_AGG_DEDUP_EVENTS; i++) {
        const wspr_agg_event_t* e = &agg_events[i];
        if (!e->used) continue;
        const int32_t dbr = (int32_t)(start_br - e->start_br);
        if (dbr >= -WSPR_AGG_SLOT_TOL && dbr <= WSPR_AGG_SLOT_TOL &&
            ring_col_gap(col, e->col0) <= WSPR_AGG_COL_MATCH)
            return true;
    }
    return false;
}

static void agg_event_record(uint32_t start_br, int16_t col) {
    wspr_agg_event_t* e = &agg_events[agg_event_next % WSPR_AGG_DEDUP_EVENTS];
    agg_event_next++;
    e->start_br = start_br;
    e->col0 = col;
    e->used = true;
}

static void agg_reset_track(wspr_agg_track_t* t) {
    memset(t, 0, sizeof(*t));
}

// One gate-failing integrated decode (metric / agreement / unpack): charge
// the track's attempt budget. Past WSPR_AGG_MAX_FAILED_DECODES - or at the
// WSPR_AGG_MAX_COUNT depth cap - the track resets, so neither the failure
// counter nor count can grow without bound.
static int agg_decode_fail(wspr_agg_track_t* t) {
    t->failed++;
    if (t->failed >= WSPR_AGG_MAX_FAILED_DECODES || t->count >= WSPR_AGG_MAX_COUNT)
        agg_reset_track(t);
    return 0;
}

// Message-consistency test between a new slot's weights and the track's
// accumulated weights: the PEARSON correlation over the 162 symbol weights,
// with both means removed, in pure integer arithmetic (the decoder is
// freestanding - no libm). Same message: the two weight vectors share their
// tone pattern and correlate clearly positively even at -26 dB; a different
// payload (message or station change on one column) decorrelates to ~0.
// Mean removal matters: the demod weights carry a positive bias, so raw
// sign agreement does NOT separate two different messages (both "agree"
// well above the 50% chance level). Used to refuse merging A-then-B
// payloads BEFORE any decode: a frequency match (col +-2) is not station
// identity, so this cheap consistency gate is the only per-slot payload
// identity evidence the accumulator has.
//
// Threshold: corr >= 7/20 = 0.35 accepted (see the calibration note at the
// constants); tested as num*20 > 7*sqrt(da*db)
// with num/da/db the integer Pearson sums scaled by WSPR_CODED_BITS, so no
// division is needed. No variance on either side = no evidence: allow.
// Threshold 7/20 = 0.35, calibrated on the host suites (DIAG cases): the
// same station's slot pair correlates 0.55..0.98 down to -26 dB WSJT (min
// observed 0.55), while a DIFFERENT payload on the same column stays within
// +-0.10 and ring-path junk candidates against a real track peaked at 0.224
// over 149 samples - so 0.35 sits more than double the junk ceiling below
// the real-signal floor. Re-measure the DIAG cases after touching the demod.
#define WSPR_AGG_MSG_CORR_NUM 7
#define WSPR_AGG_MSG_CORR_DEN 20
static int64_t sqrt_i64(int64_t v) {
    if (v <= 0) return 0;
    int64_t x = v;
    int64_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + v / x) / 2;
    }
    return x;
}
static bool agg_message_consistent(const int16_t* acc, const int8_t* slot) {
    int32_t sum_a = 0, sum_b = 0;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        sum_a += acc[n];
        sum_b += slot[n];
    }
    // Mean-removed sums, scaled by WSPR_CODED_BITS to stay in integers.
    int64_t num = 0, da = 0, db = 0;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int64_t a = (int64_t)WSPR_CODED_BITS * acc[n] - sum_a;
        const int64_t b = (int64_t)WSPR_CODED_BITS * slot[n] - sum_b;
        num += a * b;
        da += a * a;
        db += b * b;
    }
    if (da == 0 || db == 0) return true;  // no evidence: allow the merge
    return num * WSPR_AGG_MSG_CORR_DEN >
           (int64_t)WSPR_AGG_MSG_CORR_NUM * sqrt_i64(da) * sqrt_i64(db);
}

void wspr_decode_reset_aggregation(void) {
#if defined(WSPR_RX_EARLY_REPORTS)
    pending_early_track = NULL;
#endif
    memset(agg_tracks, 0, sizeof(agg_tracks));
    memset(agg_events, 0, sizeof(agg_events));
    agg_event_next = 0;
}

/*
 * Age the track table in SLOT units against the stream position `now_br` (the
 * newest absolute block-row the caller has processed). Absolute, NOT incremental:
 * the age of a track is recomputed from its own last_slot_br, so the call is
 * idempotent and its effect does not depend on how often it is made — a run of
 * candidate jobs inside one slot leaves the age at 0, and a silent period ages
 * every track by one slot per 120 s even when no candidate job fires at all.
 */
void wspr_agg_age_to(uint32_t now_br) {
    for (int i = 0; i < WSPR_AGG_TRACKS; i++) {
        wspr_agg_track_t* t = &agg_tracks[i];
        if (!t->active) continue;
        // Wrap-safe signed difference FIRST, division in the WIDE type, and
        // only the final clamp+cast narrows to uint8: a long silent period
        // (or a uint32 producer wrap) must never wrap the age counter.
        const int32_t d = (int32_t)(now_br - t->last_slot_br);
        uint32_t idle = d > 0 ? (uint32_t)d / (uint32_t)WSPR_AGG_SLOT_ROWS : 0;
        if (idle > 0) idle--;  // the slot holding the contribution itself is not idle
        if (idle > 255) idle = 255;
        if (idle > t->age) t->age = (uint8_t)idle;  // monotone: never un-age
        if (t->age > WSPR_AGG_TRACK_MAX_AGE) agg_reset_track(t);
    }
}

// 8-bit bit-reversal, the address walk of the documented interleave.
static uint8_t bit_reverse_8(uint8_t v) {
    v = (uint8_t)(((v & 0x55) << 1) | ((v >> 1) & 0x55));
    v = (uint8_t)(((v & 0x33) << 2) | ((v >> 2) & 0x33));
    v = (uint8_t)(((v & 0x0F) << 4) | ((v >> 4) & 0x0F));
    return v;
}

// Integer square root (Newton), so the variance normalizer needs no floats.
static int sqrt_i32(int32_t v) {
    if (v <= 0) return 0;
    int32_t x = v;
    int32_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + v / x) / 2;
    }
    return x;
}

static uint8_t parity32(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return (uint8_t)(v & 1u);
}

static uint8_t bit_at(const uint8_t* path, int index) {
    return (uint8_t)((path[index >> 3] >> (7 - (index & 7))) & 1u);
}

static void set_bit(uint8_t* path, int index, uint8_t value) {
    const uint8_t mask = (uint8_t)(0x80u >> (index & 7));
    if (value) {
        path[index >> 3] |= mask;
    } else {
        path[index >> 3] &= (uint8_t)~mask;
    }
}

// Register states for the first `depth` path bits, mirroring the encoder.
static void path_registers(const uint8_t* path, int depth, uint32_t* reg0, uint32_t* reg1) {
    uint32_t r0 = 0, r1 = 0;
    for (int i = 0; i < depth; i++) {
        const uint32_t b = i < WSPR_PAYLOAD_BITS ? bit_at(path, i) : 0;
        r0 = (r0 << 1) | b;
        r1 = (r1 << 1) | b;
    }
    *reg0 = r0;
    *reg1 = r1;
}

// Max-heap keyed on metric, so the best partial path pops in log time. When
// the pool is full the lowest-metric node is evicted instead of dropping the
// newcomer: the flood of a low-SNR search then cannot push out the useful
// frontier. The eviction scan only runs on pushes into a full pool, which
// typical decodes never reach.
static void heap_replace_min(int16_t metric, uint8_t depth, const uint8_t* path) {
    int mi = 0;
    for (int i = 1; i < node_count; i++) {
        if (search_workspace.nodes[i].metric < search_workspace.nodes[mi].metric) mi = i;
    }
    if (search_workspace.nodes[mi].metric >= metric) return;
    search_workspace.nodes[mi].metric = metric;
    search_workspace.nodes[mi].depth = depth;
    memcpy(search_workspace.nodes[mi].path, path, WSPR_NODE_PATH_BYTES);
    int i = mi;
    while (i > 0) {
        const int parent = (i - 1) / 2;
        if (search_workspace.nodes[parent].metric >= search_workspace.nodes[i].metric) break;
        const wspr_stack_node_t tmp = search_workspace.nodes[parent];
        search_workspace.nodes[parent] = search_workspace.nodes[i];
        search_workspace.nodes[i] = tmp;
        i = parent;
    }
    while (true) {
        int best = i;
        const int l = 2 * i + 1;
        const int r = 2 * i + 2;
        if (l < node_count && search_workspace.nodes[l].metric > search_workspace.nodes[best].metric) best = l;
        if (r < node_count && search_workspace.nodes[r].metric > search_workspace.nodes[best].metric) best = r;
        if (best == i) break;
        const wspr_stack_node_t tmp = search_workspace.nodes[best];
        search_workspace.nodes[best] = search_workspace.nodes[i];
        search_workspace.nodes[i] = tmp;
        i = best;
    }
}

static void heap_push(int16_t metric, uint8_t depth, const uint8_t* path) {
    if (node_count >= WSPR_STACK_NODES) {
        heap_replace_min(metric, depth, path);
        return;
    }
    int i = node_count++;
    search_workspace.nodes[i].metric = metric;
    search_workspace.nodes[i].depth = depth;
    memcpy(search_workspace.nodes[i].path, path, WSPR_NODE_PATH_BYTES);
    while (i > 0) {
        const int parent = (i - 1) / 2;
        if (search_workspace.nodes[parent].metric >= search_workspace.nodes[i].metric) break;
        const wspr_stack_node_t tmp = search_workspace.nodes[parent];
        search_workspace.nodes[parent] = search_workspace.nodes[i];
        search_workspace.nodes[i] = tmp;
        i = parent;
    }
}

static void heap_pop(wspr_stack_node_t* out) {
    *out = search_workspace.nodes[0];
    node_count--;
    if (node_count == 0) return;
    search_workspace.nodes[0] = search_workspace.nodes[node_count];
    int i = 0;
    while (true) {
        int best = i;
        const int l = 2 * i + 1;
        const int r = 2 * i + 2;
        if (l < node_count && search_workspace.nodes[l].metric > search_workspace.nodes[best].metric) best = l;
        if (r < node_count && search_workspace.nodes[r].metric > search_workspace.nodes[best].metric) best = r;
        if (best == i) break;
        const wspr_stack_node_t tmp = search_workspace.nodes[best];
        search_workspace.nodes[best] = search_workspace.nodes[i];
        search_workspace.nodes[i] = tmp;
        i = best;
    }
}

/*
 * Sequential decode of one candidate's 162 soft weights. Returns true and
 * fills the 81 source bits when a full path survives the effort limit.
 *
 * The per-symbol metric is the soft correlation plus a constant Fano-style
 * bias: increment = +-(weight) - R. R does not change the ordering of
 * complete paths (it sums to the same 162*R everywhere), it only shapes the
 * search: a wrong branch then loses metric on every step, so the junk
 * frontier falls away instead of climbing alongside the correct path, which
 * is what a bare correlation metric does at low SNR.
 *
 * Completed paths are kept and the search continues while some leaf can
 * still beat the best completion (branch and bound over the tight
 * per-symbol bound max(0, |w| - R)): the first completion of a bare
 * max-first stack is frequently a near-miss impostor, the best completion
 * is the maximum-likelihood path within the explored effort.
 */
static bool stack_decode(const int8_t* weights, uint8_t out_bits[WSPR_SOURCE_BYTES], int16_t* out_metric) {

    uint8_t path[WSPR_NODE_PATH_BYTES];
    memset(path, 0, sizeof(path));

    // Bias: half the mean |weight|, at least 1. All-zero weights (no signal
    // at all) leave every increment at zero and the search just dives.
    int32_t abs_sum = 0;
    for (int i = 0; i < WSPR_CODED_BITS; i++) {
        abs_sum += weights[i] >= 0 ? weights[i] : (int32_t)-weights[i];
    }
    const int bias = abs_sum > 0 ? (int)(abs_sum / (2 * WSPR_CODED_BITS)) : 0;
    const int r_bias = bias < 1 ? 1 : bias;

    // Tight admissible remaining-gain bound per depth.
    suffix_gain[WSPR_SOURCE_BITS] = 0;
    for (int i = WSPR_SOURCE_BITS - 1; i >= 0; i--) {
        const int g0 = weights[2 * i] >= 0 ? weights[2 * i] : (int)-weights[2 * i];
        const int g1 = weights[2 * i + 1] >= 0 ? weights[2 * i + 1] : (int)-weights[2 * i + 1];
        const int g0n = g0 > r_bias ? g0 - r_bias : 0;
        const int g1n = g1 > r_bias ? g1 - r_bias : 0;
        const int gain = suffix_gain[i + 1] + g0n + g1n;
        suffix_gain[i] = (int16_t)(gain > 32767 ? 32767 : gain);
    }

    node_count = 0;
    heap_push(0, 0, path);

    int best_complete = 0;
    bool have_best = false;  // the bound only engages after a completion
    int effort = 0;
    while (node_count > 0 && effort < WSPR_STACK_EFFORT) {
        wspr_stack_node_t node;
        heap_pop(&node);
        // A node that cannot beat the best completion is dropped. This is a
        // continue, not a break: the bound depends on the depth, so a
        // shallower node with a larger remaining gain may still win.
        if (have_best && node.metric + suffix_gain[node.depth] <= best_complete) continue;
        if (node.depth == WSPR_SOURCE_BITS) {
            best_complete = node.metric;
            have_best = true;
            memset(out_bits, 0, WSPR_SOURCE_BYTES);
            memcpy(out_bits, node.path, WSPR_NODE_PATH_BYTES);
            if (out_metric) *out_metric = node.metric;
            continue;
        }
        effort++;
        if (node.metric < -WSPR_METRIC_LIMIT) continue;

        uint32_t reg0, reg1;
        path_registers(node.path, node.depth, &reg0, &reg1);
        const int child_bound = have_best ? best_complete - suffix_gain[node.depth + 1] : (-32768);
        // The 31 termination bits are fixed zero, not additional payload.
        // Searching both values here fits noise with an invalid codeword.
        const uint8_t choices = node.depth < WSPR_PAYLOAD_BITS ? 2 : 1;
        for (uint8_t b = 0; b < choices; b++) {
            const uint32_t r0 = (reg0 << 1) | b;
            const uint32_t r1 = (reg1 << 1) | b;
            int m = node.metric;
            if (weights[2 * node.depth]) {
                m += (parity32(r0 & WSPR_POLY_0) ? weights[2 * node.depth] : -weights[2 * node.depth]) - r_bias;
            }
            if (weights[2 * node.depth + 1]) {
                m += (parity32(r1 & WSPR_POLY_1) ? weights[2 * node.depth + 1] : -weights[2 * node.depth + 1]) - r_bias;
            }
            if (m < -WSPR_METRIC_LIMIT) continue;
            if (m <= child_bound) continue;
            uint8_t child_path[WSPR_NODE_PATH_BYTES];
            memcpy(child_path, node.path, WSPR_NODE_PATH_BYTES);
            if (node.depth < WSPR_PAYLOAD_BITS) set_bit(child_path, node.depth, b);
            heap_push((int16_t)m, (uint8_t)(node.depth + 1), child_path);
        }
    }
    return have_best;
}

// Undo the bit-reversal interleave: the channel symbols carry the coded bits
// in the documented interleaved order, and the stack decoder reads them in the
// encoder's natural (poly0, poly1) order. Used by the per-candidate demod and
// by the cluster decode, which must walk the interleave the SAME way.
static void deinterleave_weights(const int8_t* sym, int8_t* bits) {
    int p = 0;
    for (unsigned i = 0; i < 256; i++) {
        const unsigned j = bit_reverse_8((uint8_t)i);
        if (j < WSPR_CODED_BITS) {
            bits[p++] = sym[j];
        }
    }
}

// Re-encoding consistency against the nonzero hard decisions. This measures
// agreement with a fitted codeword, not an independent confidence probability;
// sync, termination, payload and repeat validation remain necessary.
static int reencode_agreement(const uint8_t* source) {
    uint8_t reenc[WSPR_CODED_BITS];
    wspr_encode_bits(source, reenc);
    int nonzero = 0;
    int disagree = 0;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        if (sym_weight[n] == 0) continue;
        nonzero++;
        if ((sym_weight[n] > 0) != (reenc[n] != 0)) disagree++;
    }
    return nonzero > 0 ? (100 * (nonzero - disagree)) / nonzero : 0;
}

static int8_t estimate_snr_snapshot(const wspr_snapshot_t* snap, const uint8_t* source);

/*
 * Candidate-path shared internals. The window decoder's stage 2 and the
 * snapshot candidate API (below) run the SAME refinement, demodulation,
 * decode, gates, SNR and accumulation code, which is what makes the two
 * paths bit-identical for one candidate.
 */

// Column of symbol n's 4-bin block: col(n) = col0 + drift*n/161. Integer
// division truncates toward zero, matching the encoder of estimate_snr's
// exclusion band and the historical demod expression.
static int sym_col(int col0, int drift, int n) {
    int col = col0;
    if (drift != 0) {
        col += (drift * n) / (WSPR_CODED_BITS - 1);
    }
    return col;
}

static int pair_sigma(int32_t sum_pair, int32_t sum_sq) {
    const int count_pair = 2 * WSPR_CODED_BITS;
    const int mean = (int)(sum_pair / count_pair);
    const int32_t variance = sum_sq / count_pair - mean * mean;
    return variance > 1 ? (int)sqrt_i32(variance) : 1;
}

// Weight of one symbol: pair difference normalized by sigma, rounded to
// nearest, clipped to +-6.
static void quantize_pair(int e0, int e1, int sigma, int8_t* out) {
    int w = e1 - e0;
    w = (w >= 0) ? ((2 * w + sigma) / (2 * sigma))
                 : -((2 * -w + sigma) / (2 * sigma));
    if (w > 6) w = 6;
    if (w < -6) w = -6;
    *out = (int8_t)w;
}

// Soft demodulation straight from the matrix: the window decoder's stage-2
// demod, factored out so the snapshot path and the tests read the exact same
// code. Returns false if the drifting pair walk leaves the matrix.

// Soft demodulation from the snapshot's frozen 4-bin symbol rows: e0 is the
// sync-selected bit-0 bin, e1 the bit-1 bin - the same bytes the matrix demod
// would read, so the weights are bit-identical by construction (the test
// still compares them byte for byte).
static bool demod_weights_snapshot(const wspr_snapshot_t* snap, int8_t* sym_weight) {
    if (snap->meta.valid_symbols != WSPR_CODED_BITS) return false;
    int32_t sum_pair = 0;
    int32_t sum_sq = 0;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int e0 = snap->tones[n][wspr_sync_vector[n]];
        const int e1 = snap->tones[n][wspr_sync_vector[n] + 2];
        sum_pair += e0 + e1;
        sum_sq += e0 * e0 + e1 * e1;
    }
    const int sigma = pair_sigma(sum_pair, sum_sq);
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        quantize_pair(snap->tones[n][wspr_sync_vector[n]],
                      snap->tones[n][wspr_sync_vector[n] + 2], sigma, &sym_weight[n]);
    }
    return true;
}

/*
 * The candidate pipeline AFTER demodulation (sym_weight holds the candidate's
 * weights): pre-gate, deinterleave, stack decode, re-encode agreement,
 * unpack, SNR (matrix or snapshot), then the report metric gate. Returns true
 * and fills *out when the candidate becomes a report. `snap != NULL` selects
 * the snapshot SNR source and ignores the matrix arguments.
 *
 * This is the WINDOW decoder's per-candidate path only. The ring path never
 * reports a per-window decode: its candidates feed the slot-aligned track
 * accumulator instead (see wspr_agg_track_contribute below).
 *
 * Test-only: compiled only when the window decoder (also test-only) is.
 */

// Magnitudes use byte = 20*log10(power) + 240. Convert relative to byte 255
// without pulling libm into the external image. All exponents are -43..0.
static float magnitude_power(uint8_t value) {
    const float x = ((int)value - 255) * 0.166096405f;
    int exponent = (int)x;
    if (x < exponent) --exponent;
    const float f = x - exponent;
    union {
        uint32_t u;
        float f;
    } scale;
    scale.u = (uint32_t)(exponent + 127) << 23;
    return scale.f * (1.0f + f * (0.69314718f + f * (0.24022651f + f * (0.05550411f + f * 0.00961813f))));
}

// power_log2() is the WSPR-local libm-free series in wspr_fast_math.h. It avoids
// pulling libm's log into the external image, and its arithmetic is identical
// to the front ends' log2_from_bits() (see the header).

static int8_t estimate_snr_finish(float tone, int noise_count, const uint16_t* histogram);

// SNR referenced to 2500 Hz, as used by WSPR. Average linear powers, subtract
// the noise in the decoded tone bin, and scale its fs/N bin width to 2500 Hz.
// A median over distant bins rejects other stations and converts to the
// mean Gaussian-noise power. Timing/bin offsets can still bias this estimate.
//
// The exclusion band [c-4, c+7] tracks the DRIFT: c = col0 + drift*n/161 per
// symbol (integer division truncates toward zero for negative drift, the
// same expression in the matrix path and in the snapshot fill, so both
// exclude exactly the same bins).
// Test-only: only the test-only window candidate pipeline calls it.

// Shared median/noise-power tail of estimate_snr. The snapshot path calls it
// with the tone sum read from the frozen tone bytes and the histogram
// accumulated at snapshot time - identical inputs, identical result.
static int8_t estimate_snr_finish(float tone, int noise_count, const uint16_t* histogram) {
    if (!noise_count) return -50;
    // Median rejects other WSPR stations and their skirts. For complex Gaussian
    // noise median(power) = ln(2)*mean(power): add 1.59 dB (3.18 byte units).
    int below = 0, bucket = 0;
    const int middle = noise_count / 2;
    while (bucket < 63 && below + histogram[bucket] < middle)
        below += histogram[bucket++];
    const float fraction = histogram[bucket] ? (float)(middle - below) / histogram[bucket] : 0;
    int noise_byte = (int)(4 * bucket + 4 * fraction + 3.1835f + 0.5f);
    if (noise_byte > 255) noise_byte = 255;
    const float noise = magnitude_power((uint8_t)noise_byte);
    const float signal = tone / WSPR_CODED_BITS - noise;
    if (signal <= 0) return -50;
    const float ratio = signal / noise * ((float)WSPR_SAMPLE_RATE / (WSPR_SAMPLES_PER_SYMBOL * 2500.0f));
    const float db = 3.01029996f * power_log2(ratio);
    int rounded = (int)(db + (db >= 0 ? 0.5f : -0.5f));
    if (rounded < -50) rounded = -50;
    if (rounded > 49) rounded = 49;
    return (int8_t)rounded;
}

// SNR of a completed decode read entirely from the snapshot: the tone sum
// over the frozen 4-bin symbol rows and the histogram accumulated when the
// snapshot was taken. Bit-identical to estimate_snr on the same candidate.
static int8_t estimate_snr_snapshot(const wspr_snapshot_t* snap, const uint8_t* source) {
    uint8_t* coded = (uint8_t*)bit_weight;
    wspr_encode_bits(source, coded);
    float tone = 0;
    int noise_count = 0;
    for (int n = 0; n < WSPR_CODED_BITS; ++n) {
        tone += magnitude_power(snap->tones[n][wspr_sync_vector[n] + 2 * coded[n]]);
    }
    for (int b = 0; b < WSPR_NOISE_HIST_BINS; ++b) {
        noise_count += snap->noise_histogram[b];
    }
    return estimate_snr_finish(tone, noise_count, snap->noise_histogram);
}

/*
 * Slot-aligned weight accumulator (see the block comment on the track table).
 *
 * Called once per released ring candidate: `sym_weight` holds the snapshot's
 * 162 soft weights, `snap` its metadata (column, drift, refined excess, SNR
 * inputs), `start_br` the candidate's absolute start block-row. The weights
 * fold into the matching track; when the track holds two or more slots, one
 * integrated decode is attempted ON THE SUM through the same acceptance
 * gates as any other decode (report metric, re-encode agreement, unpack).
 * Returns 1 and fills *out when the integrated decode REPORTS; the track
 * then resets so the next slot starts a fresh cycle. An early report keeps
 * the track and its first-slot weights available for accumulation.
 */
#if defined(WSPR_RX_EARLY_REPORTS)
static bool stability_check(const wspr_snapshot_t*, const wspr_message_t*);
#endif
int wspr_agg_track_contribute(const wspr_snapshot_t* snap,
                              const int8_t* sym_weight_in,
                              uint32_t start_br,
                              wspr_decode_result_t* out) {
#if defined(WSPR_RX_EARLY_REPORTS)
    pending_early_track = NULL;
#endif
    if (snap == NULL || sym_weight_in == NULL || out == NULL) return 0;
    const int16_t col = snap->meta.col0;

    // Event dedup BEFORE any track math: the same transmission's ghost anchors
    // (other banks, drift-shifted starts within the plateau) contribute nothing,
    // even after the track was reset by a successful decode in between. See the
    // block comment on agg_events.
    if (agg_event_duplicate(start_br, col)) {
        return 0;
    }

    // Self-aging: every candidate job carries the stream position, so the
    // track ages stay current even if the receiver glue never calls
    // wspr_agg_age_to() between jobs (idempotent absolute recomputation).
    wspr_agg_age_to(start_br);

    // Match the contribution against EVERY frequency-matching track (not just
    // the first: a junk track on a neighbouring column must not shadow the
    // real station's track - that first-match shadowing silently swallowed a
    // real slot when a noise candidate had opened a track one column off).
    // A track can absorb the slot only if BOTH its slot-window accepts the
    // candidate AND the message-consistency correlation passes; among the
    // accepting tracks prefer the deepest integration (that is the station's
    // own track, not a coincidence).
    wspr_agg_track_t* near = NULL;   // any active col-matching track
    wspr_agg_track_t* merge = NULL;  // window-accepting AND message-consistent
    for (int i = 0; i < WSPR_AGG_TRACKS; i++) {
        wspr_agg_track_t* cand = &agg_tracks[i];
        if (!cand->active || cand->col0 < col - WSPR_AGG_COL_MATCH ||
            cand->col0 > col + WSPR_AGG_COL_MATCH)
            continue;
        near = cand;
        const int32_t dbr = (int32_t)(start_br - cand->last_slot_br);
        const int32_t k = (dbr + WSPR_AGG_SLOT_ROWS / 2) / WSPR_AGG_SLOT_ROWS;
        const int32_t off = dbr - k * WSPR_AGG_SLOT_ROWS;
        // Slot window: the wrap-safe block-row gap must land within +-20
        // block-rows of a whole number of WSPR slots (120 s = 351.5625
        // block-rows). Anything closer than one slot is the same
        // transmission's anchor family; anything in between is junk.
        if (k < 1 || off < -WSPR_AGG_SLOT_TOL || off > WSPR_AGG_SLOT_TOL) {
            continue;
        }
        if (!agg_message_consistent(cand->weight, sym_weight_in)) {
            continue;
        }
        if (merge == NULL || cand->count > merge->count) merge = cand;
    }
    wspr_agg_track_t* t = merge;
    bool yielded = false;
    if (t != NULL) {
        // Age in SLOTS against the chosen track: a gap of k slot periods since
        // its last contribution means k-1 empty slots passed. Set ABSOLUTELY
        // (the same figure wspr_agg_age_to derives from the stream position).
        // Past the limit the track resets and this contribution re-opens it.
        const int32_t dbr = (int32_t)(start_br - t->last_slot_br);
        const int32_t k = (dbr + WSPR_AGG_SLOT_ROWS / 2) / WSPR_AGG_SLOT_ROWS;
        const uint32_t idle = k > 1 ? (uint32_t)(k - 1) : 0;
        if (idle > t->age) t->age = idle > 255 ? 255 : (uint8_t)idle;
        if (t->age > WSPR_AGG_TRACK_MAX_AGE) {
            agg_reset_track(t);
            t->col0 = col;
            t->anchor_start_br = start_br;
        }
    } else if (near != NULL) {
        // A col-matching track exists, but none can absorb this candidate
        // (slot-window or message-consistency failed). By default REJECT and
        // leave every track untouched: a message change (A then B on one
        // column) costs a rejection, never a mixed payload, and a noise
        // candidate must not reset or shadow the station's integration.
        // BOUNDED RECOVERY: a track that is stale past the TTL, or that has
        // now rejected WSPR_AGG_TRACK_YIELD_REJECTS strong-grade (excess >=
        // open floor) candidates from DISTINCT transmissions, YIELDS - it
        // resets (no weight ever crosses between messages) and this candidate
        // opens a fresh cycle. Without this, a junk- or old-payload track on
        // the frequency would block a new station until the full TTL; with
        // it, recovery costs two strong slots plus one integration slot.
        // Weak candidates never charge the counter, so the scan's junk flood
        // cannot evict a live track this way; and several scan hypotheses
        // (other banks, drift-shifted starts) of ONE rejected transmission
        // charge it at most once, via the per-track last-rejected onset
        // identity (WSPR_AGG_REJECT_TX_TOL) - one strong interference burst
        // must never evict a healthy integration.
        const bool strong = snap->meta.sync_score >= WSPR_AGG_OPEN_EXCESS_MIN;
        for (int i = 0; i < WSPR_AGG_TRACKS; i++) {
            wspr_agg_track_t* cand = &agg_tracks[i];
            if (!cand->active || cand->col0 < col - WSPR_AGG_COL_MATCH ||
                cand->col0 > col + WSPR_AGG_COL_MATCH)
                continue;
            if (cand->age > WSPR_AGG_TRACK_MAX_AGE) {
                agg_reset_track(cand);
                yielded = true;
                continue;
            }
            if (!strong) continue;
            // T09-C2.3 ghost-grade charge gate (union of the two
            // trace-motivated predicates, each shown necessary in isolation:
            // C2.1 proved the same-slot-family condition protects the weak
            // opener-re-anchor case F4_cwop_02; C2.2's half-grade band showed
            // which rejects are ghost-grade; F7 g353 showed new-slot weak
            // rejects must keep charging). SKIP the charge only when the
            // rejector is BOTH far below the occupant's last accepted excess
            // (less than half -> ghost grade vs a real station) AND inside the
            // slot-window's own same-slot family band (k<1: by the window
            // gate's definition a hypothesis of the slot just accepted, which
            // could never merge anyway). All other rejects charge as baseline.
            {
                const int32_t dbr2 = (int32_t)(start_br - cand->last_slot_br);
                const int32_t k2 =
                    (dbr2 + WSPR_AGG_SLOT_ROWS / 2) / WSPR_AGG_SLOT_ROWS;
                if (2 * snap->meta.sync_score < cand->last_sync_score && k2 < 1)
                    continue;
            }
            if (cand->has_reject) {
                const int32_t rbr = (int32_t)(start_br - cand->last_reject_br);
                if (rbr >= -WSPR_AGG_REJECT_TX_TOL && rbr <= WSPR_AGG_REJECT_TX_TOL &&
                    ring_col_gap(col, cand->last_reject_col) <= WSPR_AGG_REJECT_TX_COL_TOL)
                    continue;  // another hypothesis of the same rejected TX
            }
            cand->last_reject_br = start_br;
            cand->last_reject_col = col;
            cand->has_reject = true;
            cand->rejected_strong++;
            if (cand->rejected_strong >= WSPR_AGG_TRACK_YIELD_REJECTS) {
                agg_reset_track(cand);
                yielded = true;
            }
        }
        if (!yielded) {
            return 0;
        }
    }
    if (t == NULL && !yielded) {
        if (snap->meta.sync_score < WSPR_AGG_OPEN_EXCESS_MIN) {
            // Junk-grade candidate with nothing nearby: drop before it can
            // pollute the table.
            return 0;
        }
        /* T09-C1: removed the (near != NULL) refuse block here: control-flow
           dead (audit RC2) - a col-matching track with a refused merge either
           charged a yield above or returned 0 from the !yielded branch, so at
           this point near is always NULL. */
    }
    if (t == NULL) {
        // Open a new track (nothing nearby, or the occupant just yielded): a
        // free slot first, else evict - tracks idle longer than
        // WSPR_AGG_TRACK_MAX_AGE slots first, then the one with the fewest
        // accumulated slots (idle age breaks ties). See the
        // WSPR_AGG_OPEN_EXCESS_MIN note: junk-grade candidates never get
        // here, so the scan's excess-5 noise flood cannot occupy the table.
        int best = -1;
        for (int i = 0; i < WSPR_AGG_TRACKS; i++) {
            wspr_agg_track_t* cand = &agg_tracks[i];
            if (!cand->active) {
                best = i;
                break;
            }
            if (best < 0) {
                best = i;
                continue;
            }
            const bool cand_stale = cand->age > WSPR_AGG_TRACK_MAX_AGE;
            const bool best_stale = agg_tracks[best].age > WSPR_AGG_TRACK_MAX_AGE;
            if (cand_stale != best_stale) {
                if (cand_stale) best = i;
                continue;
            }
            if (cand->count < agg_tracks[best].count ||
                (cand->count == agg_tracks[best].count && cand->age > agg_tracks[best].age)) {
                best = i;
            }
        }
        t = &agg_tracks[best];
        agg_reset_track(t);
        t->col0 = col;
        t->anchor_start_br = start_br;
    }
    // t != NULL here means the slot MERGES into its station's track (or
    // re-opens a TTL-expired one after the reset above): fall through to the
    // fold. The junk-grade drop for the no-nearby case already returned at the
    // floor check above, so nothing else can reach the fold.

    // Fold the slot's weights in (T09-C1: plain add; the saturating arm
    // was dead). INVARIANT, constant-coupled (audit U7): one slot weight is
    // +-6 max (snapshot demod quantizer, quantize_pair) and a track holds at
    // most WSPR_AGG_MAX_COUNT (8) slots, so |weight| <= 48 - far inside both
    // int16 (no wrap) and the int8 decode scale (no clamp). If the quantizer
    // range or the count cap grows, restore saturation/clamping here.
    t->active = true;
    t->rejected_strong = 0;  // the yield counter is per streak, not per track
    t->has_reject = false;   // the streak's TX identity resets with it
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        t->weight[n] += sym_weight_in[n];
    }
    t->count++;  // capped: the decode below resets the track once this
                 // reaches WSPR_AGG_MAX_COUNT, so the uint8 never wraps
    t->age = 0;
    t->last_slot_br = start_br;
    t->last_sync_score = snap->meta.sync_score;
    t->last_drift = snap->meta.drift;
    t->last_bank = snap->meta.bank;
    // Accepted contribution: record the packet identity so its ghost anchors can
    // never weigh the same TX again (see agg_events).
    agg_event_record(start_br, col);

#if defined(WSPR_RX_EARLY_REPORTS)
    const bool early = t->count == 1 && snap->meta.sync_score >= 12;
#else
    const bool early = false;
#endif
    const unsigned min_count = WSPR_AGG_MIN_COUNT;
    if (t->count < min_count && !early) {
        return 0;
    }

    // Integrated decode of the SUM, through the same gates as any decode.
    // T09-C1: the defensive +-127 clamp was provably dead - with +-6 slot
    // weights and the 8-slot cap (see the fold invariant note) the sum fits
    // int8 with room to spare, so the cast is exact.
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        sym_weight[n] = (int8_t)t->weight[n];
    }
    deinterleave_weights(sym_weight, bit_weight);

    uint8_t source[WSPR_SOURCE_BYTES] = {0};
    int16_t metric = 0;
    // Attempt budget bookkeeping: a completion that FAILS a gate counts
    // against WSPR_AGG_MAX_FAILED_DECODES (a stream of impostor completions
    // on one column must not keep a garbage track alive forever); a run that
    // completes no path at all is plain weak signal and is bounded by the
    // count cap instead. At the count cap every failed attempt is terminal:
    // the track resets, so count can never exceed WSPR_AGG_MAX_COUNT.
    if (!stack_decode(bit_weight, source, &metric)) {
        if (t->count >= WSPR_AGG_MAX_COUNT || t->failed >= WSPR_AGG_MAX_FAILED_DECODES)
            agg_reset_track(t);
        return 0;
    }
    if (metric < (early ? 30 : WSPR_REPORT_METRIC_MIN)) {
        return early ? 0 : agg_decode_fail(t);
    }
    const int agreement = reencode_agreement(source);
    if (agreement < (early ? 90 : WSPR_MIN_AGREEMENT_PCT)) {
        return early ? 0 : agg_decode_fail(t);
    }

    wspr_message_t message;
    if (!wspr_unpack_message(source, &message)) {
        return early ? 0 : agg_decode_fail(t);
    }

#if defined(WSPR_RX_EARLY_REPORTS)
    if (!early && t->early_valid &&
        memcmp(t->early_source, source, sizeof(t->early_source)) == 0) {
        // This decode reuses the shown slot. Keep scanner dedup, but send
        // no second UI report that could falsely confirm this payload.
        agg_reset_track(t);
        return WSPR_CONTRIBUTION_ALREADY_REPORTED;
    }
#endif
    memset(out, 0, sizeof(*out));
    memcpy(out->callsign, message.callsign, sizeof(message.callsign));
    memcpy(out->grid, message.grid, sizeof(message.grid));
    out->dbm = message.dbm;
    // SNR of the integrated decode, read from this slot's snapshot: the tone
    // sum over the decoded bins and the noise histogram frozen at snapshot
    // time. Integration raises decoding confidence, not the RF SNR figure of
    // a single slot, so no integration gain is added here.
    out->snr_db = estimate_snr_snapshot(snap, source);
    out->freq_hz =
        (int16_t)(snap->meta.bin0_hz + (col * WSPR_SAMPLE_RATE) / WSPR_SAMPLES_PER_SYMBOL);
    // The newest slot's refined drift and sync excess: the accumulator has no
    // alignment scan of its own.
    out->drift_bins = (int16_t)t->last_drift;
    out->sync_score = t->last_sync_score;
    out->row_offset = (int16_t)start_br;
    out->bank = t->last_bank;
    out->metric = metric;
    out->agreement_pct = (int8_t)agreement;

#if defined(WSPR_RX_EARLY_REPORTS)
    if (early) {
        if (!stability_check(snap, &message)) {
            return WSPR_CONTRIBUTION_NONE;
        }
        memcpy(t->early_source, source, sizeof(t->early_source));
        pending_early_track = t;
        return WSPR_CONTRIBUTION_EARLY_REPORT;
    }
#endif

    // Successful decode: reset the track. The next slot of the same station
    // opens a fresh cycle, so a message change is never mixed with the old
    // weights and the same evidence cannot report twice.
    agg_reset_track(t);
    return 1;
}

int wspr_snapshot_demod_weights(const wspr_snapshot_t* snap, int8_t* sym_weight) {
    if (snap == NULL || sym_weight == NULL) return 0;
    return demod_weights_snapshot(snap, sym_weight) ? 1 : 0;
}

/*
 * ---- Ring scan / ring snapshot (ring architecture, stage 3; see wspr_decode.h) ----
 *
 * These readers walk the ring by ABSOLUTE block-row id: symbol n of a packet at id S
 * lives in block-row S + 2n, physical slot (S % block_rows + 2n) % block_rows. The
 * modulo-of-id-then-add order is what keeps the addressing correct across both the
 * physical ring seam and a uint32 counter wrap (2^32 is not a multiple of 352, so
 * (S + 2n) % block_rows on RAW ids would alias near the wrap).
 *
 * Ownership (single M4 core): the producer writes one new row per ~341 ms and publishes
 * it by incrementing total_br AFTER the row bytes. Readers never trust a latched
 * counter across a job: every scan job gets a FRESH cursor read from its caller, the
 * snapshot re-reads the LIVE cursor (wspr_ring_cursor_fn) before AND after each row
 * copy, and any candidate that reached its overwrite deadline (or may have been
 * rewritten mid-copy) is dropped and counted, never decoded. The exact ordering
 * guarantees - and their single-core limits - are documented next to
 * wspr_ring_cursor_fn in wspr_decode.h.
 */

// Physical-slot pointer of absolute block-row br. uint32 arithmetic is natural here
// and MUST stay raw: the producer writes slot = id % block_rows on the same wrapping
// uint32 ids, so (base + off) % block_rows computed in uint32 is exactly the slot the
// producer used, on both sides of a counter wrap. Reducing the id first and adding the
// offset afterwards would DISAGREE with the producer near 2^32 (2^32 is not a multiple
// of 352) and read the wrong rows. See the ownership note in wspr_portapack.h for the
// once-per-2^32-rows overwrite-margin seam this implies.
static const uint8_t* ring_row(const uint8_t* mag, int stride_bins, uint32_t block_rows, uint32_t br) {
    return mag + (size_t)(br % block_rows) * (size_t)stride_bins;
}

// Load the live producer cursor through the caller's accessor, or fall back to the
// caller's one-shot value when no accessor was provided (host/unit callers that know
// the producer is quiescent). The barrier keeps the compiler from moving later byte
// loads above this cursor load.
static uint32_t ring_cursor_load(wspr_ring_cursor_fn cursor_fn, void* cursor_ctx, uint32_t total_br) {
    uint32_t cur = total_br;
    if (cursor_fn) cur = cursor_fn(cursor_ctx);
    WSPR_RING_COMPILER_BARRIER();
    return cur;
}

// Guard for ONE row read: the row `br` must be published (cursor past it) and its
// physical slot must not have been rewritten (the rewriting row br + block_rows not
// yet published: while it is being written, total_br still equals br + block_rows, so
// the safe window is cursor - br <= block_rows - 1).
static bool ring_row_safe(uint32_t cur, uint32_t br, uint32_t block_rows) {
    const int32_t age = (int32_t)(cur - br);
    return age >= 1 && age <= (int32_t)(block_rows - 1);
}

// sync_score over the ring: same balanced two-parity correlation as the matrix
// sync_score, but symbol n reads block-row start_br + 2n through the ring seam. `count`
// leading symbols are summed (162 for a full packet). Returns the balanced selected
// score on the full-162 scale, *out_mean = the balanced opposite-pair background, so
// score/162 - mean is the signed excess. Rows are guaranteed inside the ring by the
// caller's total_br check.
static int32_t ring_sync_score(const uint8_t* mag, int num_bins, uint32_t block_rows, uint32_t start_br, int col0, int drift, int count, int32_t* out_mean) {
    int32_t score[2] = {0, 0};
    int32_t opposite_score[2] = {0, 0};
    int pair_count[2] = {0, 0};
    for (int n = 0; n < count; n++) {
        const uint8_t* row =
            ring_row(mag, num_bins, block_rows, start_br + 2u * (uint32_t)n);
        int col = col0;
        if (drift != 0) {
            col += (drift * (int)n) / (WSPR_CODED_BITS - 1);
        }
        if (col < 0 || col + 3 >= num_bins) return -1;
        const int opposite = col + 1 - wspr_sync_vector[n];
        const uint8_t o0 = row[opposite];
        const uint8_t o1 = row[opposite + 2];
        const int group = wspr_sync_vector[n];
        ++pair_count[group];
        opposite_score[group] += o0 > o1 ? o0 : o1;
        col += wspr_sync_vector[n];
        const uint8_t e0 = row[col];
        const uint8_t e1 = row[col + 2];
        score[group] += (e0 > e1) ? e0 : e1;
    }
    const int denominator = 2 * pair_count[0] * pair_count[1];
    if (out_mean)
        *out_mean = (opposite_score[0] * pair_count[1] + opposite_score[1] * pair_count[0]) / denominator;
    return (score[0] * pair_count[1] + score[1] * pair_count[0]) * WSPR_CODED_BITS / denominator;
}

// BELT-AND-BRACES recomputation of the sync excess FROM THE FROZEN TONES (the
// snapshot's 4 bins per symbol at the chosen drift), exactly reproducing
// ring_sync_score's balanced two-parity formula on the same data: symbol n's
// selected bins are tones[n][sync] / tones[n][sync+2], the opposite-parity
// background tones[n][1-sync] / tones[n][3-sync] (the snapshot froze scratch
// row columns c..c+3 with c = sym_col(col0, drift, n)). Returns the RAW balanced
// score (the x-WSPR_CODED_BITS scale of ring_sync_score; divide by
// WSPR_CODED_BITS and subtract *out_mean to get the excess scale the scan and
// the gates use), or -1 if a derived column leaves the band (cannot happen for
// a validated col0/drift, but a refused hypothesis must never gate anything).
static int32_t tones_sync_excess(const wspr_snapshot_t* snap, int col0, int drift, int num_bins, int32_t* out_mean) {
    int32_t score[2] = {0, 0};
    int32_t opposite_score[2] = {0, 0};
    int pair_count[2] = {0, 0};
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int c = sym_col(col0, drift, n);
        if (c < 0 || c + 3 >= num_bins) return -1;
        const int group = wspr_sync_vector[n];
        const uint8_t o0 = snap->tones[n][1 - group];
        const uint8_t o1 = snap->tones[n][3 - group];
        ++pair_count[group];
        opposite_score[group] += o0 > o1 ? o0 : o1;
        const uint8_t e0 = snap->tones[n][group];
        const uint8_t e1 = snap->tones[n][group + 2];
        score[group] += (e0 > e1) ? e0 : e1;
    }
    const int denominator = 2 * pair_count[0] * pair_count[1];
    if (out_mean)
        *out_mean = (opposite_score[0] * pair_count[1] + opposite_score[1] * pair_count[0]) / denominator;
    return (score[0] * pair_count[1] + score[1] * pair_count[0]) * WSPR_CODED_BITS / denominator;
}

// Column distance on the linear band axis (columns never wrap in frequency).
static int ring_col_gap(int a, int b) {
    int d = a - b;
    return d < 0 ? -d : d;
}

// Smallest ring the scan/snapshot can work with: a full packet spans 323 block-rows
// (symbols at start + 2n), and the overwrite deadline needs its margin on top. An
// OSR1 producer publishes one block-row per symbol row, so its 176-row ring can
// never meet this bound — the ring functions reject it (see the header: OSR2-only).
#define WSPR_RING_MIN_BLOCK_ROWS (WSPR_CODED_BITS * 2 - 1 + WSPR_RING_DEADLINE_ROWS)

int wspr_ring_scan_start(const uint8_t* mag,
                         int num_bins,
                         uint32_t block_rows,
                         uint32_t total_br,
                         uint32_t start_br,
                         int min_excess,
                         int max_results,
                         wspr_ring_cursor_fn cursor_fn,
                         void* cursor_ctx,
                         int16_t* out_cols,
                         int16_t* out_excess,
                         int8_t* out_drifts) {
    if (mag == NULL || num_bins < 8 || block_rows == 0) return 0;
    if (num_bins > WSPR_RING_SNAP_SCRATCH_BINS) return 0;
    if (block_rows < (uint32_t)WSPR_RING_MIN_BLOCK_ROWS) return 0;  // OSR1 ring: rejected
    if (max_results <= 0) max_results = 1;

    // STALENESS BRACKET, entry side: read the LIVE cursor before the first byte.
    // The correlation below takes no shortcuts - it reads the live matrix for the
    // whole packet span (~134 columns x 7 drifts x 162 symbols) and can be preempted
    // mid-pass by the higher-priority producer, so a one-shot counter latched by the
    // caller proves nothing about what is still in the ring by the time we finish.
    // cursor_fn == NULL falls back to the caller's one-shot total_br (host callers
    // with a quiescent producer): the bracket then never fires, exactly like the
    // snapshot's guarded reader.
    const uint32_t cur0 = ring_cursor_load(cursor_fn, cursor_ctx, total_br);
    // A full packet must be published: symbol 161 lives in block-row start + 322,
    // and ids are published while cursor > id. Wrap-safe signed difference. (Checked
    // on the REFRESHED cursor; a cursor behind the caller's value can only mean the
    // caller misread, and refusing is always safe.)
    if ((int32_t)(cur0 - start_br) < WSPR_CODED_BITS * 2 - 1) return 0;
    // Already past the safe window before we start? Nothing here can fix that.
    if ((int32_t)(cur0 - start_br) > (int32_t)(block_rows - 1)) return -1;

    // Stage-1 searches (col, drift) hypotheses: a transmission with linear drift
    // smears its tones across neighboring bins at drift 0, and the balanced
    // excess at the wrong drift is a fraction of the aligned one - a drift-0-only
    // scan drops such packets before the stage-2 refinement ever runs (this was
    // the pre-ring decoder's limitation too). The single pass keeps the best
    // excess PER COLUMN; the ranked top-K output below costs no extra correlation.
    int16_t col_excess[WSPR_RING_SNAP_SCRATCH_BINS];
    int8_t col_drift[WSPR_RING_SNAP_SCRATCH_BINS];
    for (int c = 0; c < num_bins; c++) col_excess[c] = INT16_MIN;
    const int cmax = num_bins - 3;  // the correlation needs col + 3 < num_bins
    for (int c = 0; c < cmax; c++) {
        for (int d = WSPR_DRIFT_MIN; d <= WSPR_DRIFT_MAX; d++) {
            int32_t mean = 0;
            const int32_t score = ring_sync_score(mag, num_bins, block_rows, start_br, c,
                                                  d, WSPR_CODED_BITS, &mean);
            if (score < 0) continue;
            const int32_t excess = score / WSPR_CODED_BITS - mean;
            if (excess > (int32_t)col_excess[c]) {
                col_excess[c] = (int16_t)excess;
                col_drift[c] = (int8_t)d;
            }
        }
    }

    // STALENESS BRACKET, exit side: the producer may have published any number of
    // rows WHILE the correlation ran (a preempting higher-priority writer rewrites
    // the ring slots the scan is still reading). If the refreshed cursor shows that
    // any row of the packet span could have been overwritten or torn (cursor -
    // start_br > block_rows - 1, the same ring_row_safe window the snapshot's
    // guarded copies use), the col/excess/drift results above describe rewritten
    // bytes: return -1 and let the caller discard the whole scan (stale_scans)
    // BEFORE it can become a pending/secondary candidate. This closes the read
    // window with the proven single-core publication protocol; the HONEST residual
    // assumption is that a healthy scan finishes well inside one block-row (~341 ms)
    // so it is never falsely stale - that scan wall time is NOT yet measured on ARM,
    // so do not declare the race closed until it is.
    const uint32_t cur1 = ring_cursor_load(cursor_fn, cursor_ctx, total_br);
    if ((int32_t)(cur1 - start_br) > (int32_t)(block_rows - 1)) return -1;

    // Greedy excess-ranked top-K with +-2-column non-maximum suppression: one
    // transmission's sync plateau spans several columns, so the neighbours of a
    // picked column are masked; a STATION 3+ columns away survives and is ranked
    // on its own excess. Two simultaneous starts therefore both surface instead
    // of the weaker one being silently swallowed by the single best column (QR5).
    int found = 0;
    while (found < max_results) {
        int best = -1;
        for (int c = 0; c < cmax; c++) {
            if (col_excess[c] < min_excess) continue;
            if (best < 0 || col_excess[c] > col_excess[best]) best = c;
        }
        if (best < 0) break;
        if (out_cols) out_cols[found] = (int16_t)best;
        if (out_excess) out_excess[found] = col_excess[best];
        if (out_drifts) out_drifts[found] = col_drift[best];
        found++;
        for (int c = best - 2; c <= best + 2; c++) {
            if (c >= 0 && c < num_bins) col_excess[c] = INT16_MIN;
        }
    }
    return found;
}

void wspr_ring_scan_init(wspr_ring_scan_state_t* st) {
    if (st == NULL) return;
    st->next_scan_br = 0;
    memset(st->anchors, 0, sizeof(st->anchors));
    st->pending_br = 0;
    st->pending_col = 0;
    st->pending_excess = 0;
    st->pending_drift = 0;
    st->pending_valid = false;
    st->scanned = 0;
    st->expired = 0;
    st->scan_lag = 0;
    st->max_lag = 0;
    st->snap_drops = 0;
    st->stale_scans = 0;
    memset(st->secondary, 0, sizeof(st->secondary));
}

void wspr_ring_scan_mark_decoded(wspr_ring_scan_state_t* st, uint32_t start_br, int16_t col0) {
    if (st == NULL) return;
    // Record the decode as a suppression anchor. Keep up to WSPR_RING_MAX_ANCHORS
    // recent decodes: one anchor per STATION, so simultaneous stations never erase
    // each other's ghosts and a fresh anchor only evicts the oldest entry.
    int victim = 0;
    for (int i = 0; i < WSPR_RING_MAX_ANCHORS; i++) {
        if (!st->anchors[i].valid) {
            victim = i;
            break;
        }
        // Same packet already anchored (a repeated report of the same evidence):
        // refresh the entry in place, keep the others.
        if (st->anchors[i].br == start_br) {
            victim = i;
            break;
        }
        if (st->anchors[victim].valid &&
            (int32_t)(st->anchors[i].br - st->anchors[victim].br) < 0)
            victim = i;
    }
    st->anchors[victim].br = start_br;
    st->anchors[victim].col = col0;
    st->anchors[victim].valid = true;
    // The decoded packet's ghost window is settled: a held candidate inside it is dead.
    // A held candidate OUTSIDE it is a different station (released earlier only because
    // its column differs) and must survive the mark so it decodes on the next poll.
    if (st->pending_valid) {
        const int32_t d = (int32_t)(st->pending_br - start_br);
        const bool ghost =
            d >= -20 && d <= 20 && ring_col_gap(st->pending_col, col0) <= 2;
        if (ghost) st->pending_valid = false;
    }
    // The decode's ghost window settles the stashed secondary candidates too.
    for (int i = 0; i < WSPR_RING_MAX_SECONDARY; i++) {
        if (!st->secondary[i].valid) continue;
        const int32_t d = (int32_t)(st->secondary[i].br - start_br);
        if (d >= -20 && d <= 20 && ring_col_gap(st->secondary[i].col, col0) <= 2)
            st->secondary[i].valid = false;
    }
}

// Ghost check of a start hypothesis against one decoded anchor: within +-20
// block-rows of the anchor ON the anchor's own column +-2 — the sync plateau of
// the same transmission. A candidate on a different column is a different station
// and never suppressed (the old +-4-any-column rule killed simultaneous stations).
static bool ring_anchor_ghost(const wspr_ring_anchor_t* a, uint32_t s, int col) {
    if (!a->valid) return false;
    const int32_t d = (int32_t)(s - a->br);
    return d >= -20 && d <= 20 && ring_col_gap(col, a->col) <= 2;
}

// The scan hands candidates out no later than this many block-rows behind the
// cursor; the snapshot keeps its harder plain-deadline gate, so the difference
// (WSPR_RING_RELEASE_MARGIN_ROWS) is the worker's preemption allowance between
// the release and its snapshot.
#define WSPR_RING_RELEASE_LIMIT_ROWS(block_rows) \
    ((block_rows) - WSPR_RING_DEADLINE_ROWS - WSPR_RING_RELEASE_MARGIN_ROWS)

// Stash one SECONDARY candidate of a start (a different station sharing the
// onset): +-2-column NMS against the pending candidate and the already-stashed
// ones, excess-ranked (valid entries sorted strongest-first; invalid holes sink
// to the tail). Bounded: a full queue keeps its strongest entries.
static bool ring_sec_before(const wspr_ring_secondary_t* a, const wspr_ring_secondary_t* b) {
    // true when a sorts before (ahead of) b: valid first, then excess desc.
    if (a->valid != b->valid) return a->valid;
    if (!a->valid) return false;
    return a->excess >= b->excess;
}
static void ring_secondary_push(wspr_ring_scan_state_t* st, uint32_t br, int16_t col, int16_t excess, int8_t drift) {
    if (st->pending_valid && ring_col_gap(col, st->pending_col) < 3) return;
    for (int i = 0; i < WSPR_RING_MAX_SECONDARY; i++) {
        if (st->secondary[i].valid && ring_col_gap(col, st->secondary[i].col) < 3) return;
    }
    // Victim: an invalid slot, else the weakest (last sorted) entry; a new
    // candidate weaker than every stashed one is dropped, never stalls the queue.
    int slot = -1;
    int weakest = -1;
    for (int i = 0; i < WSPR_RING_MAX_SECONDARY; i++) {
        if (!st->secondary[i].valid) {
            slot = i;
            break;
        }
        if (weakest < 0 || st->secondary[i].excess < st->secondary[weakest].excess)
            weakest = i;
    }
    if (slot < 0) {
        if (excess <= st->secondary[weakest].excess) return;
        slot = weakest;
    }
    st->secondary[slot].br = br;
    st->secondary[slot].col = col;
    st->secondary[slot].excess = excess;
    st->secondary[slot].drift = drift;
    st->secondary[slot].valid = true;
    // Insertion sort (K <= 3): valid entries first, excess descending.
    for (int i = 1; i < WSPR_RING_MAX_SECONDARY; i++) {
        wspr_ring_secondary_t tmp = st->secondary[i];
        int j = i;
        while (j > 0 && ring_sec_before(&tmp, &st->secondary[j - 1])) {
            st->secondary[j] = st->secondary[j - 1];
            j--;
        }
        st->secondary[j] = tmp;
    }
}

int wspr_ring_scan_poll(const uint8_t* mag,
                        int num_bins,
                        uint32_t block_rows,
                        uint32_t total_br,
                        wspr_ring_scan_state_t* st,
                        uint32_t max_starts,
                        wspr_ring_cursor_fn cursor_fn,
                        void* cursor_ctx,
                        uint32_t* out_start_br,
                        int16_t* out_col,
                        int16_t* out_excess,
                        int8_t* out_drift) {
    if (mag == NULL || st == NULL || num_bins < 8 || block_rows == 0) return 0;
    if (block_rows < (uint32_t)WSPR_RING_MIN_BLOCK_ROWS) return 0;  // OSR1 ring: rejected
    if (max_starts == 0) max_starts = WSPR_RING_SCAN_MAX_STARTS_PER_POLL;
    uint32_t starts_done = 0;
    const int32_t release_limit = (int32_t)WSPR_RING_RELEASE_LIMIT_ROWS(block_rows);

    for (;;) {
        // Pending candidate first, in deadline order: a pending held through a
        // backlog (the producer ran ahead while the previous decode ran) must be
        // EXPIRED, not released — releasing it would hand the caller a candidate
        // whose rows may already be overwritten. Expiry is checked BEFORE release,
        // against the RELEASE LIMIT (harder than the snapshot's own deadline gate,
        // so the released candidate carries the margin as preemption allowance).
        if (st->pending_valid) {
            const uint32_t pending_lag = total_br - st->pending_br;
            if ((int32_t)pending_lag > release_limit) {
                st->pending_valid = false;
                st->expired++;
                st->scan_lag = pending_lag;
                if (pending_lag > st->max_lag) st->max_lag = pending_lag;
                continue;
            }
            // Release the held candidate once its +-2 block-row comparison window is
            // fully scannable (both neighbours plus one row of slack). This is what
            // ranks one packet's ghost anchors against each other: only the sharpest
            // alignment is ever handed to the decode. Same-TX ghosts beyond this
            // window are rejected by the slot accumulator's slot-window acceptance
            // (see wspr_agg_track_contribute), not here - a wider holdback would
            // hand the decode a drift-shifted misalignment instead of the true one.
            if ((int32_t)pending_lag >= WSPR_CODED_BITS * 2 + 3) {
                st->pending_valid = false;
                st->scan_lag = pending_lag;
                if (pending_lag > st->max_lag) st->max_lag = pending_lag;
                if (out_start_br) *out_start_br = st->pending_br;
                if (out_col) *out_col = st->pending_col;
                if (out_excess) *out_excess = st->pending_excess;
                if (out_drift) *out_drift = st->pending_drift;
                return 1;
            }
        }

        // Secondary (different-station) candidates of already-scanned starts, in
        // excess order: released before new starts are scanned so they never lose
        // their slot to fresh work, with the same expiry-before-release gate.
        {
            int sec = -1;
            for (int i = 0; i < WSPR_RING_MAX_SECONDARY; i++) {
                if (st->secondary[i].valid) {
                    sec = i;
                    break;
                }
            }
            if (sec >= 0) {
                const uint32_t sec_lag = total_br - st->secondary[sec].br;
                if ((int32_t)sec_lag > release_limit) {
                    st->secondary[sec].valid = false;
                    st->expired++;
                    st->scan_lag = sec_lag;
                    if (sec_lag > st->max_lag) st->max_lag = sec_lag;
                    continue;
                }
                const wspr_ring_secondary_t emit = st->secondary[sec];
                for (int i = sec + 1; i < WSPR_RING_MAX_SECONDARY; i++)
                    st->secondary[i - 1] = st->secondary[i];
                st->secondary[WSPR_RING_MAX_SECONDARY - 1].valid = false;
                if (out_start_br) *out_start_br = emit.br;
                if (out_col) *out_col = emit.col;
                if (out_excess) *out_excess = emit.excess;
                if (out_drift) *out_drift = emit.drift;
                return 1;
            }
        }

        // Bounded progress: one call consumes at most max_starts hypotheses. A
        // backlog that piled up behind a seconds-long FEC decode cannot wedge the
        // decode thread in a single unbounded loop; the caller re-polls while the
        // producer keeps moving or the thread is re-woken, so the scan always
        // catches up (bounded work per call, unbounded progress overall).
        if (starts_done >= max_starts) return 0;

        // A start becomes scannable when its LAST symbol row (start + 322) is
        // published: total_br - start >= 323. One new start per completed block-row.
        if ((int32_t)(total_br - st->next_scan_br) < WSPR_CODED_BITS * 2 - 1) {
            return 0;
        }
        const uint32_t s = st->next_scan_br++;
        starts_done++;
        st->scan_lag = total_br - s;
        if (st->scan_lag > st->max_lag) st->max_lag = st->scan_lag;

        // Release limit: scanning a start any later could only produce candidates
        // that expire before their release; drop it here (counted).
        if ((int32_t)(total_br - s) > release_limit) {
            st->expired++;
            continue;
        }

        st->scanned++;
        // ONE correlation pass per start; the ranked top-K columns (distinct
        // stations sharing the onset) fall out of the per-column best already
        // computed there. The strongest runs the pending best-of-window ranking;
        // the others wait in the bounded secondary queue.
        int16_t cols[WSPR_RING_MAX_SECONDARY + 1];
        int16_t excesses[WSPR_RING_MAX_SECONDARY + 1];
        int8_t drifts[WSPR_RING_MAX_SECONDARY + 1];
        const int found = wspr_ring_scan_start(mag, num_bins, block_rows, total_br, s,
                                               WSPR_RING_MIN_EXCESS,
                                               WSPR_RING_MAX_SECONDARY + 1,
                                               cursor_fn, cursor_ctx,
                                               cols, excesses, drifts);
        if (found < 0) {
            // The scan raced the producer: rows of this start's span were rewritten
            // while the correlation ran, so its col/excess/drift describe garbage.
            // Discard BEFORE anything can enter pending/secondary and count it -
            // this is the scan's counterpart of the snapshot's OVERWRITTEN drop.
            st->stale_scans++;
            continue;
        }
        if (found <= 0) continue;

        // Suppression by the decoded packets' anchors (see the header): the sync
        // correlation plateau of one transmission spans several symbols of start shift
        // around its own frequency column, and those ghost anchors must not re-decode
        // (or re-weigh) the same packet. Suppression is COLUMN-AWARE and only against
        // the anchor's own family: another station starting at the same time on a
        // different frequency is a separate candidate. A start one full slot later
        // (352 block-rows = the 120 s WSPR repeat cadence) is the station's next
        // transmission and scans normally.
        int primary = -1;
        for (int k = 0; k < found; k++) {
            bool ghost = false;
            for (int i = 0; i < WSPR_RING_MAX_ANCHORS && !ghost; i++) {
                ghost = ring_anchor_ghost(&st->anchors[i], s, cols[k]);
            }
            if (ghost) continue;
            if (primary < 0) {
                primary = k;  // strongest surviving column runs the pending ranking
                continue;
            }
            ring_secondary_push(st, s, cols[k], excesses[k], drifts[k]);
        }
        if (primary < 0) continue;
        const int16_t col = cols[primary];
        const int16_t excess = excesses[primary];
        const int8_t drift = drifts[primary];

        // Hold or compare against the pending best-of-window candidate.
        if (!st->pending_valid) {
            st->pending_valid = true;
            st->pending_br = s;
            st->pending_col = col;
            st->pending_excess = excess;
            st->pending_drift = drift;
            continue;
        }
        const int32_t dp = (int32_t)(s - st->pending_br);
        if (dp > 2) {
            // Outside the comparison window: release the held candidate now and
            // hold this one as the next window's seed.
            const uint32_t emit_br = st->pending_br;
            const int16_t emit_col = st->pending_col;
            const int16_t emit_excess = st->pending_excess;
            const int8_t emit_drift = st->pending_drift;
            st->pending_br = s;
            st->pending_col = col;
            st->pending_excess = excess;
            st->pending_drift = drift;
            if (out_start_br) *out_start_br = emit_br;
            if (out_col) *out_col = emit_col;
            if (out_excess) *out_excess = emit_excess;
            if (out_drift) *out_drift = emit_drift;
            return 1;
        }
        // Inside the comparison window: a candidate >= 3 columns away is a DIFFERENT
        // station, not a ghost of the held packet - release the held one immediately
        // so both decode. Otherwise keep the sharper alignment.
        if (ring_col_gap(col, st->pending_col) >= 3) {
            const uint32_t emit_br = st->pending_br;
            const int16_t emit_col = st->pending_col;
            const int16_t emit_excess = st->pending_excess;
            const int8_t emit_drift = st->pending_drift;
            st->pending_br = s;
            st->pending_col = col;
            st->pending_excess = excess;
            st->pending_drift = drift;
            if (out_start_br) *out_start_br = emit_br;
            if (out_col) *out_col = emit_col;
            if (out_excess) *out_excess = emit_excess;
            if (out_drift) *out_drift = emit_drift;
            return 1;
        }
        if (excess > st->pending_excess) {
            st->pending_br = s;
            st->pending_col = col;
            st->pending_excess = excess;
            st->pending_drift = drift;
        }
    }
}

int wspr_ring_snapshot_candidate(const uint8_t* mag,
                                 int num_bins,
                                 uint32_t block_rows,
                                 uint32_t total_br,
                                 uint32_t start_br,
                                 int col0,
                                 int bin0_hz,
                                 int8_t scan_drift,
                                 int16_t scan_excess,
                                 wspr_snapshot_t* snap,
                                 wspr_ring_cursor_fn cursor_fn,
                                 void* cursor_ctx,
                                 int* out_fail) {
    (void)scan_excess;  // Snapshot validation recomputes the score from captured tones.
    if (out_fail) *out_fail = WSPR_SNAP_BAD_ARGS;
    if (mag == NULL || snap == NULL) return 0;
    if (num_bins < 8 || block_rows == 0) return 0;
    if (num_bins > WSPR_RING_SNAP_SCRATCH_BINS) return 0;
    if (block_rows < (uint32_t)WSPR_RING_MIN_BLOCK_ROWS) return 0;  // OSR1 ring: rejected
    if (col0 < 0 || col0 + 3 >= num_bins) return 0;

    // The drift was refined during the incremental scan (stage 1 now searches
    // (col, drift) hypotheses); the snapshot freezes the tones at that drift.
    const int drift = scan_drift;

    // GUARDED READER, live cursor: the caller's total_br may be seconds old after a
    // long FEC decode; every gate below runs on a REFRESHED read. First refresh
    // BEFORE any byte is touched: is the full packet published, and is it still
    // inside the overwrite deadline?
    uint32_t cur = ring_cursor_load(cursor_fn, cursor_ctx, total_br);
    // Full packet published? Symbol 161 lives in block-row start + 322.
    if ((int32_t)(cur - start_br) < WSPR_CODED_BITS * 2 - 1) return 0;
    // Overwrite expiry BEFORE the first read: once the producer is within the
    // deadline margin of the packet's oldest row, the freeze could race the writer.
    // Reject instead of reading rewritten bytes; the scan's own deadline check is the
    // first gate, this is the second (the refreshed one).
    if ((int32_t)(cur - start_br) > (int32_t)(block_rows - WSPR_RING_DEADLINE_ROWS)) {
        if (out_fail) *out_fail = WSPR_SNAP_EXPIRED;
        return 0;
    }

    // Freeze the 4-bin symbol rows at the chosen drift and accumulate the noise
    // histogram over the same rows and the same exclusion band [c-4, c+7] that
    // estimate_snr uses — the wrap-safe ring walk of wspr_snapshot_candidate.
    //
    // Each row is copied in ONE guarded step: refresh cursor -> guard -> copy the
    // whole row into local scratch -> refresh cursor again -> guard must still hold.
    // On the single-core M4 a preempting producer that rewrites this row's slot
    // during the copy MUST publish the rewriting row (increment past br + block_rows)
    // before the reader runs again, so a post-copy guard that still holds proves the
    // copied bytes were not torn (see wspr_ring_cursor_fn's protocol note in
    // wspr_decode.h for the exact guarantees and their single-core limits).
    memset(snap, 0, sizeof(*snap));
#if defined(WSPR_RX_EARLY_REPORTS)
    snap->has_neighbors = true;
#endif
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int c = sym_col(col0, drift, n);
        if (c < 0 || c + 3 >= num_bins) return 0;  // disqualify, like stage 2
        const uint32_t row_br = start_br + 2u * (uint32_t)n;
        cur = ring_cursor_load(cursor_fn, cursor_ctx, total_br);
        if (!ring_row_safe(cur, row_br, block_rows)) {
            if (out_fail) *out_fail = WSPR_SNAP_OVERWRITTEN;
            return 0;
        }
        const uint8_t* row = ring_row(mag, num_bins, block_rows, row_br);
        uint8_t scratch[WSPR_RING_SNAP_SCRATCH_BINS];
        memcpy(scratch, row, (size_t)num_bins);
        cur = ring_cursor_load(cursor_fn, cursor_ctx, total_br);
        if (!ring_row_safe(cur, row_br, block_rows)) {
            if (out_fail) *out_fail = WSPR_SNAP_OVERWRITTEN;
            return 0;
        }
        for (int i = 0; i < 4; i++) snap->tones[n][i] = scratch[c + i];
#if defined(WSPR_RX_EARLY_REPORTS)
        if (c < 1 || c + 4 >= num_bins) {
            snap->has_neighbors = false;
        } else {
            snap->neighbor[n][0] = scratch[c - 1];
            snap->neighbor[n][1] = scratch[c + 4];
        }
#endif
        for (int b = 0; b < num_bins; b++) {
            if (b >= c - 4 && b <= c + 7) continue;
            ++snap->noise_histogram[scratch[b] >> 2];
        }
    }
    // CHEAP BELT before the snapshot can feed the accumulator: recompute the sync
    // excess FROM THE FROZEN TONES for the chosen drift and gate on it. This does
    // NOT replace the scan's staleness bracket (the whole scan can still be stale
    // and is discarded there); it guarantees the gate the candidate passed actually
    // matches the bytes that are about to be contributed - a mismatched or weak
    // frozen correlation never reaches the weights. Cost: one 162-symbol walk over
    // already-frozen local data.
    {
        // tones_sync_excess returns the RAW balanced SCORE (x WSPR_CODED_BITS
        // scale, ~28000 for a strong packet); the scan excess contract
        // (wspr_decode.h) and every gate below live on the EXCESS scale
        // score / WSPR_CODED_BITS - mean, typically 5..60. Both parts come
        // back from the helper - derive the excess here, gate and record IT.
        // (An earlier revision recorded the raw score, which made the gate a
        // no-op and let junk candidates pass the accumulator's strong threshold.)
        int32_t belt_mean = 0;
        const int32_t belt_score =
            tones_sync_excess(snap, col0, drift, num_bins, &belt_mean);
        if (belt_score < 0) {
            if (out_fail) *out_fail = WSPR_SNAP_WEAK_SCORE;
            return 0;
        }
        const int32_t belt_excess = belt_score / WSPR_CODED_BITS - belt_mean;
        if (belt_excess < WSPR_RING_MIN_EXCESS) {
            if (out_fail) *out_fail = WSPR_SNAP_WEAK_SCORE;
            return 0;
        }
        snap->meta.sync_score = (int16_t)belt_excess;
    }
    snap->meta.start_row_abs = start_br;
    snap->meta.col0 = (int16_t)col0;
    snap->meta.bin0_hz = (int16_t)bin0_hz;
    snap->meta.bank = (int8_t)(start_br & 1u);
    snap->meta.drift = (int8_t)drift;
    snap->meta.valid_symbols = WSPR_CODED_BITS;
    if (out_fail) *out_fail = WSPR_SNAP_OK;
    return 1;
}

#if defined(WSPR_RX_EARLY_REPORTS)
static bool stability_check(const wspr_snapshot_t* snap, const wspr_message_t* packet) {
    if (!snap->has_neighbors) return false;
    for (int delta = -1; delta <= 1; delta += 2) {
        uint8_t pairs[WSPR_CODED_BITS][2];
        int sum = 0, sum_sq = 0;
        for (int n = 0; n < WSPR_CODED_BITS; ++n) {
            for (int i = 0; i < 2; ++i) {
                const int tone = wspr_sync_vector[n] + 2 * i;
                const int adjacent = tone + delta;
                const unsigned side = adjacent < 0 ? snap->neighbor[n][0]
                    : (adjacent > 3 ? snap->neighbor[n][1] : snap->tones[n][adjacent]);
                const unsigned value = (7u * snap->tones[n][tone] + side) >> 3;
                pairs[n][i] = (uint8_t)value;
                sum += (int)value;
                sum_sq += (int)(value * value);
            }
        }
        const int sigma = pair_sigma(sum, sum_sq);
        for (int n = 0; n < WSPR_CODED_BITS; ++n)
            quantize_pair(pairs[n][0], pairs[n][1], sigma, &sym_weight[n]);
        deinterleave_weights(sym_weight, bit_weight);
        uint8_t source[WSPR_SOURCE_BYTES];
        int16_t metric = 0;
        wspr_message_t check;
        if (stack_decode(bit_weight, source, &metric) &&
            wspr_unpack_message(source, &check) && check.dbm == packet->dbm &&
            strcmp(check.callsign, packet->callsign) == 0 &&
            strcmp(check.grid, packet->grid) == 0)
            return true;
    }
    return false;
}
#endif
