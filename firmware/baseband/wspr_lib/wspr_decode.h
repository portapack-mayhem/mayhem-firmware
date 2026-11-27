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

#ifndef _WSPR_DECODE_H_
#define _WSPR_DECODE_H_

#include <stdint.h>
#include <stdbool.h>

#include "wspr_constants.h"

/*
 * Diagnostic-counter selector. The decoder's reason code (agg_dbg_reason) and
 * the FEC-attempt counters are pure observability: no accept/reject decision
 * reads them. The ordinary firmware release (WSPR_RX_DIAG=0, defined by the
 * top-level CMake for both M0 and M4) compiles NONE of the variables, their
 * assignments, the reset lines or the accessors, so the release image carries
 * no diagnostic counter state. The diagnostic firmware build
 * (WSPR_DIAGNOSTIC=ON -> WSPR_RX_DIAG=1) and the host test library
 * (WSPR_HOST_DIAG) both keep the real values.
 */
#if (defined(WSPR_RX_DIAG) && WSPR_RX_DIAG) || defined(WSPR_HOST_DIAG)
#define WSPR_DECODE_DIAG_COUNTERS 1
#else
#define WSPR_DECODE_DIAG_COUNTERS 0
#endif

/*
 * WSPR decoder, host-first: it works on a magnitude matrix of exactly the
 * shape the baseband receiver produces, so the same code runs in host tests
 * and in firmware. The matrix holds num_rows rows of num_bins bytes, row-
 * major: mag[row * num_bins + col]. Column spacing must be exactly one tone
 * step (12000/8192 = 1.4648 Hz), like the FT8 receiver keeps its bins
 * aligned to the tone spacing.
 *
 * time_osr extends the rows into interleaved sub-row banks: block-row index
 * = time_osr * row + bank, so mag holds time_osr * num_rows rows and the
 * address of (row, col) in bank b is mag[(time_osr * row + b) * num_bins +
 * col]. time_osr = 1 is the plain single-grid matrix; time_osr = 2 is the
 * dual-bank window the receiver produces, two Goertzel grids half a symbol
 * apart, so a transmission at an arbitrary sub-row phase is never more than
 * a quarter symbol away from one bank's symbol boundaries. Stage 1 and
 * stage 2 run per bank on their own candidate lists and the results merge
 * through the shared duplicate/frequency suppression; row_offset stays in
 * symbol rows and the bank that produced the result is reported separately.
 *
 * The decode has no CRC to lean on: a candidate is accepted when its payload
 * unpacks into a valid standard message.
 */

#define WSPR_MAX_RESULTS 4

/*
 * Ring architecture (stage 3): incremental scan + snapshot decode over the RING
 * capture.
 *
 * The ring producer (wspr_portapack) keeps the last `block_rows` block-rows of the
 * stream in a circular buffer, addressed by ABSOLUTE block-row id br (monotonic uint32;
 * physical slot = br % block_rows; bank = parity of br, symbol row = br / 2). One WSPR
 * symbol occupies TWO consecutive block-rows of the same parity: symbol n of a packet
 * starting at absolute id S lives in block-row S + 2n, so a full packet spans S .. S+322
 * and becomes scannable once the producer has published total_br > S + 322.
 *
 * Every completed block-row yields exactly ONE new start hypothesis (S = total_br - 323
 * at the moment of completion), so the sync correlation runs on one start per row
 * instead of the window decoder's full (rows x columns) sweep. Ring addressing uses the
 * producer's own raw uint32 id arithmetic (slot = id % block_rows, computed on the
 * natural wrapping uint32); id COMPARISONS use only the signed difference
 * (int32_t)(a - b). See wspr_portapack.h for the once-per-2^32-rows wrap seam.
 */

/* Incremental-scan acceptance threshold: mean sync-bin excess, the same scale the
 * acquisition probe gates on but one unit above it. Calibration (host streaming
 * suite): a true -20 dB transmission scores 20-23 and on-air aligned signals 6-7,
 * while the ring's much larger per-slot start budget (324 starts x 134 columns, one
 * start per block-row, versus the probe's 29 x 133 x 2) surfaced noise completions
 * scoring exactly 4 that then complete into no-CRC impostors. Pure noise otherwise
 * stays below 4. */
#define WSPR_RING_MIN_EXCESS 5

/* Scan/snapshot deadline margin, in block-rows: a start S is only scanned while
 * total_br - S <= block_rows - WSPR_RING_DEADLINE_ROWS, i.e. at least this many rows of
 * headroom before the producer overwrites the packet's oldest row. The full ring minus a
 * packet span leaves 352 - 323 = 29 rows (~9.9 s); the margin subtracts the scan and
 * snapshot wall time so the snapshot can never race the writer into its rows. */
#define WSPR_RING_DEADLINE_ROWS 12

/* Upper bound on start hypotheses ONE wspr_ring_scan_poll() call may consume (scan,
 * skip or expire). The firmware drain loop and the host tests re-poll until the scan is
 * caught up, so the bound only has to keep ONE call - and therefore the response latency
 * of the decode thread before it re-reads the live cursor - finite even when a long
 * backlog accumulated behind a seconds-long FEC decode. */
#define WSPR_RING_SCAN_MAX_STARTS_PER_POLL 8

/*
 * Reader/producer memory protocol (single Cortex-M4 core, honest wording):
 *
 * The producer (wspr_portapack finish_row) writes a row's bytes FIRST, then publishes
 * the row by incrementing the volatile counter LAST. Readers obtain the cursor ONLY
 * through wspr_ring_cursor_fn (the volatile load plus a compiler barrier) and order
 * their byte accesses around every cursor load with WSPR_RING_COMPILER_BARRIER().
 *
 * What this GUARANTEES on the target: one core, cooperative context switches. A
 * context switch preserves program order, and the barriers stop the compiler from
 * hoisting byte loads above the cursor load (reader) or sinking byte stores below the
 * counter store (producer). So "cursor says published" implies "bytes are visible" and
 * "guard passes after the copy" implies "the copied bytes were not torn".
 * What this does NOT claim: C11 data-race-freedom (the counter is a plain volatile,
 * not _Atomic) and any protection against a different core or DMA rewriting the rows.
 * If the codebase ever moves to a multi-master or multi-core writer, this protocol
 * must be upgraded to release/acquire atomics - do not reuse it there.
 */
typedef uint32_t (*wspr_ring_cursor_fn)(void* ctx);

/* Compiler barrier for the protocol above (GCC/Clang; the firmware and the host tests
 * both build with GCC-compatible compilers). */
#if defined(__GNUC__)
#define WSPR_RING_COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#else
#define WSPR_RING_COMPILER_BARRIER() \
    do {                             \
    } while (0)
#endif

/* Why wspr_ring_snapshot_candidate() refused a candidate (out_fail reason codes). */
enum {
    WSPR_SNAP_OK = 0,          /* frozen successfully                       */
    WSPR_SNAP_BAD_ARGS = 1,    /* NULL pointers / geometry outside the ring */
    WSPR_SNAP_EXPIRED = 2,     /* packet reached the overwrite deadline     */
    WSPR_SNAP_OVERWRITTEN = 3, /* a row was (or may have been) overwritten mid-copy */
    WSPR_SNAP_WEAK_SCORE = 4   /* frozen bytes failed the recomputed sync-excess gate */
};

/* Guarded single-row copy scratch size (the receiver's row width is 137 bins; a wider
 * ring is rejected rather than copied unguarded). */
#define WSPR_RING_SNAP_SCRATCH_BINS 256

/* Release margin, in block-rows: the scan hands a candidate out no later than
 * block_rows - WSPR_RING_DEADLINE_ROWS - WSPR_RING_RELEASE_MARGIN_ROWS behind the
 * cursor. The snapshot's own hard expiry gate stays at the plain deadline margin, so
 * a worker preempted for up to WSPR_RING_RELEASE_MARGIN_ROWS block-rows (~3.4 s at 10
 * rows) between the release and its snapshot still decodes instead of finding its
 * packet expired. Delay 0 is unaffected: a candidate released on schedule sits ~327
 * block-rows behind the cursor, well inside the limit. */
#define WSPR_RING_RELEASE_MARGIN_ROWS 10

/* Bounded multi-station budget for ONE start hypothesis: besides the primary
 * best-of-column candidate the scan keeps up to this many SECONDARY candidates
 * (distinct tone-0 columns, excess-ranked, +-2-column non-maximum suppression) of
 * the same start, so two stations transmitting with one onset both reach the
 * decode instead of the weaker one being silently swallowed by the single best
 * column (QR5). One extra sync scan already computed the columns; no second pass. */
#define WSPR_RING_MAX_SECONDARY 3
typedef struct {
    uint32_t br;    /* absolute start block-row of the candidate        */
    int16_t col;    /* its tone-0 column                                */
    int16_t excess; /* its mean sync-bin excess                         */
    int8_t drift;   /* its chosen drift                                 */
    bool valid;     /* entry is meaningful                              */
} wspr_ring_secondary_t;

/* Persistent per-session scan state (the ring's deduplication and ranking
 * bookkeeping; the decode thread owns one instance). */
/* Decoded-packet anchors kept for ghost suppression: up to FOUR recent decodes,
 * each identified by absolute start and tone-0 column, so simultaneous or
 * near-simultaneous stations suppress only their OWN ghost families instead of
 * one anchor suppressing every station that happened to start nearby. */
#define WSPR_RING_MAX_ANCHORS 4
typedef struct {
    uint32_t br; /* absolute start block-row of the decode         */
    int16_t col; /* its tone-0 column                              */
    bool valid;  /* entry is meaningful                            */
} wspr_ring_anchor_t;

typedef struct {
    uint32_t next_scan_br;                             /* next start hypothesis to consider                  */
    wspr_ring_anchor_t anchors[WSPR_RING_MAX_ANCHORS]; /* recent decodes (oldest      *
                                                        * evicted first; a decode a full slot later than an  *
                                                        * anchor scans normally, so 4 covers crowded phases) */
    /* Held best-of-window candidate: the scan compares every candidate with the ones
     * at +-2 block-rows (the same packet seen one symbol row off) and only releases
     * the sharpest, mirroring the window decoder's excess ranking. Same-TX ghost
     * candidates beyond that window are rejected by the slot accumulator's
     * slot-window acceptance, not here. */
    uint32_t pending_br;
    int16_t pending_col;
    int16_t pending_excess;
    int16_t pending_drift;
    bool pending_valid;
    uint32_t scanned;     /* starts actually sync-scanned (diagnostics)         */
    uint32_t expired;     /* starts/jobs dropped past the overwrite deadline    */
    uint32_t scan_lag;    /* cursor distance of the last processed start        */
    uint32_t max_lag;     /* worst cursor distance ever processed               */
    uint32_t snap_drops;  /* candidates lost to the snapshot's expiry/race gate */
    uint32_t stale_scans; /* sync scans discarded: rows raced the writer mid-scan */
    /* Secondary candidates of a scanned start (distinct stations sharing one
     * onset), excess-ranked, emitted before new starts are scanned. */
    wspr_ring_secondary_t secondary[WSPR_RING_MAX_SECONDARY];
} wspr_ring_scan_state_t;

/* Sync-probe acquisition: the full decode only finds transmissions starting
 * in rows 0..14 (the packet must fit inside the 176-row window); the probe
 * extends the sync correlation to later start rows with fewer than all 162
 * symbols available, so the tracker can steer the NEXT window onto the packet
 * even when this one cannot decode it. Rows beyond WSPR_PROBE_MAX_ROW leave
 * fewer symbols than this and are not scanned. */
#define WSPR_PROBE_MAX_ROW 28
#define WSPR_PROBE_MIN_SYMBOLS 146
/* The start row the tracker steers a probe-anchored NEXT window onto. */
#define WSPR_PROBE_LAND_ROW 4

typedef struct {
    char callsign[13];     // up to 12 characters plus terminator
    char grid[5];          // 4 characters plus terminator
    int8_t dbm;            // 0..60
    int8_t snr_db;         // estimated SNR in a 2500 Hz reference bandwidth
    int16_t freq_hz;       // audio frequency of tone 0, relative to the dial
    int16_t drift_bins;    // total frequency walk over the window, in bins
    int16_t row_offset;    // window row the transmission starts in; the slot
                           // tracker steers this toward the middle of the slack
    int16_t sync_score;    // raw sync-sum of this candidate, for display: a
                           // quiet band scores a fraction of a noisy-signal one
    int16_t metric;        // final sequential-decode path metric: real signals
                           // accumulate a clear positive score, noise hovers
                           // around zero - the decode-acceptance gate reads this
    int8_t bank;           // sub-row bank of the decode, 0..time_osr-1; the
                           // row_offset is a symbol row of that bank's grid
    int8_t agreement_pct;  // hard-decision re-encoding agreement, not a
                           // probability that the decoded callsign is correct
} wspr_decode_result_t;

/*
 * Candidate snapshot contract (ring-architecture stage 2).
 *
 * The window decoder above runs its coarse scan, drift refinement, soft
 * demodulation, SNR estimate and multi-slot accumulation directly against
 * the live magnitude matrix. The ring receiver will instead freeze the
 * chosen candidate into a small self-contained snapshot taken from the live
 * matrix, so the (potentially seconds-long) sequential decode no longer
 * depends on the matrix staying untouched. The snapshot carries everything
 * the decode reads from the matrix for ONE candidate at its CHOSEN drift:
 *
 *   - tones[162][4]: the four pair/parity bins of every symbol, extracted
 *     at col(n) = col0 + drift*n/161 (soft demod reads [sync[n]] and
 *     [sync[n]+2]; the SNR tone bin is [sync[n] + 2*decoded[n]]);
 *   - noise_histogram[64] uint16: the byte>>2 power histogram estimate_snr
 *     accumulates over the same 162 rows and all bins OUTSIDE the excluded
 *     band [c-4, c+7] around the same drifting column c - so the SNR from
 *     the snapshot is bit-identical to the matrix path's;
 *   - metadata: absolute start row, bank, col0, bin0_hz, the drift-refined
 *     sync excess, and the number of valid symbols.
 *
 * wspr_decode_window() stays the plain window path; the candidate API adds
 * to it and shares its internals, so a snapshot decode is bit-for-bit
 * equivalent to decoding the same (row, bank, col, drift) candidate
 * through the window - verified by the host equivalence test.
 */
#define WSPR_NOISE_HIST_BINS 64

typedef struct {
    uint32_t start_row_abs; /* absolute row id of symbol 0 in the stream */
    int16_t col0;           /* tone-0 column of the candidate            */
    int16_t bin0_hz;        /* audio frequency of column 0               */
    int16_t sync_score;     /* drift-refined excess, result scale        */
    int8_t bank;            /* sub-row bank the candidate came from      */
    int8_t drift;           /* chosen drift, -3..+3                      */
    uint8_t valid_symbols;  /* symbols present; a full packet is 162     */
    uint8_t reserved;       /* padding, zeroed                           */
} wspr_snapshot_meta_t;

typedef struct {
    uint8_t tones[WSPR_CODED_BITS][4];
#if defined(WSPR_RX_EARLY_REPORTS)
    uint8_t neighbor[WSPR_CODED_BITS][2];
    bool has_neighbors;
#endif
    uint16_t noise_histogram[WSPR_NOISE_HIST_BINS];
    wspr_snapshot_meta_t meta;
} wspr_snapshot_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Freeze ONE candidate out of the live matrix. (start_row, bank, col0) come
 * from the caller's coarse scan; drift refinement over -3..+3 runs here, on
 * the same excess scale as the window decoder's stage 2, and the chosen
 * drift plus tones, noise histogram and metadata are written to *snap.
 * Returns 1 on success, 0 on bad arguments or a geometry that leaves the
 * matrix (the window decoder would skip such a candidate). No wrap
 * handling: this stage still reads the live matrix, the ring producer comes
 * later.
 */
int wspr_snapshot_candidate(const uint8_t* mag,
                            int num_rows,
                            int num_bins,
                            int time_osr,
                            int start_row,
                            int bank,
                            int col0,
                            int bin0_hz,
                            uint32_t start_row_abs,
                            wspr_snapshot_t* snap);

/*
 * ---- Host-test / fixture API ----
 *
 * The window decoder, the sync-only probe and the matrix/candidate accessors
 * below are NOT linked into firmware: the firmware call graph goes through the
 * ring scan + snapshot + accumulator instead. They and their definitions are
 * compiled only when WSPR_DECODE_TEST_API is defined (the host test library
 * defines it; the firmware build does not). One source of truth, two build
 * surfaces: no parallel implementation exists.
 */

/*
 * Decode one frozen candidate the RING way: soft demod from the snapshot ->
 * slot-aligned weight accumulation (see wspr_agg_track_contribute) -> an
 * INTEGRATED decode once its track holds two or more slots. A single slot
 * never reports (the per-window stack decode at low SNR is unreliable and
 * its impostor completions are exactly what this accumulator exists to
 * suppress), so 0 is the normal result for a first-slot candidate.
 * Returns the number of results written (0 or 1).
 */
#ifdef WSPR_DECODE_TEST_API
int wspr_decode_candidate(const wspr_snapshot_t* snap,
                          wspr_decode_result_t* results,
                          int max_results);
#endif

/*
 * Soft-demodulate a snapshot into the 162 symbol-order int8 weights. The
 * equivalence test compares these bit-in-bit against the matrix demod.
 */
int wspr_snapshot_demod_weights(const wspr_snapshot_t* snap, int8_t* sym_weight);

/*
 * Soft-demodulate a FIXED (start_row, bank, col0, drift) candidate straight
 * from the matrix, the exact routine the window decoder's stage 2 uses.
 * Test/diagnostic accessor: exposes the weights the window path feeds its
 * stack decoder. Returns 1 on success, 0 if the geometry leaves the matrix.
 */
#ifdef WSPR_DECODE_TEST_API
int wspr_decode_demod_matrix_weights(const uint8_t* mag,
                                     int num_rows,
                                     int num_bins,
                                     int time_osr,
                                     int start_row,
                                     int bank,
                                     int col0,
                                     int drift,
                                     int8_t* sym_weight);
#endif

/*
 * Try to decode one capture window. mag: time_osr * num_rows * num_bins
 * uint8 magnitudes, row-major, with num_rows >= WSPR_NUM_SYMBOLS (a 120 s
 * slot capture has 176 rows for the 162-symbol transmission). time_osr: 1
 * (single row grid) or 2 (two interleaved sub-row banks). bin0_hz: audio
 * frequency of column 0. Returns the number of results written.
 */
#ifdef WSPR_DECODE_TEST_API
int wspr_decode_window(const uint8_t* mag,
                       int num_rows,
                       int num_bins,
                       int bin0_hz,
                       wspr_decode_result_t* results,
                       int max_results,
                       int time_osr);
#endif

/*
 * Sync-only acquisition probe, run AFTER a wspr_decode_window() call that
 * produced no results. Scans the sync correlation for transmission start rows
 * 0..WSPR_PROBE_MAX_ROW (the full decode only reaches 0..14) with whatever
 * symbols still fit in the window past the start (down to
 * WSPR_PROBE_MIN_SYMBOLS), at a lower score threshold and without any
 * decoding. No state is touched - in particular the multi-slot accumulation
 * table is left alone, so a sync-only window never contributes weights.
 * On success returns 1 and writes the best candidate's start row (symbol rows
 * of the bank's grid), its mean sync-bin excess (the same scale the decode's
 * sync_score field uses) and the sub-row bank it came from; out arguments may
 * be NULL.
 */
#ifdef WSPR_DECODE_TEST_API
int wspr_decode_probe_sync(const uint8_t* mag,
                           int num_rows,
                           int num_bins,
                           int time_osr,
                           int16_t* out_row,
                           int16_t* out_score,
                           int8_t* out_bank);
#endif

/*
 * Phase correction the slot tracker applies for a sync-only probe candidate
 * at start row `probe_row`: skip enough samples for the NEXT window to catch
 * the transmission starting near row 4 (a full packet plus slack). Rows at or
 * below the landing row skip nothing; the maximum is 196608 samples (16.4 s)
 * for row 28, carried by the tracker's int32 pending_skip - the int16 debug
 * telemetry saturates separately.
 */
int32_t wspr_probe_skip_samples(int probe_row);

/*
 * ---- Slot-aligned weight accumulator (multi-slot sensitivity extension) ----
 *
 * A WSPR station repeats the same message in successive 2-minute slots. Each
 * ring slot's snapshot demodulates into 162 transmission-aligned soft
 * weights; those fold into a file-static TRACK keyed by frequency column
 * (tone-0 within +-2 bins). When the track holds two or more slots, the SUM
 * of the weights is decoded once through the same metric/agreement/unpack
 * gates as any decode: the coherent integration gains ~3 dB of SNR per
 * doubling of slots, so stations below the per-window threshold still clear.
 * Per-window decodes of the ring path are never reported, and a successful
 * integrated decode resets its track (a message change starts a fresh
 * cycle). The absolute start block-row identifies the physical slot: ghost
 * anchors of the SAME transmission (within +-20 block-rows of the track's
 * newest slot on the track's column - the ring scan's own anchor-family
 * window) contribute nothing, and idle slots age the track in SLOT units
 * (more than 5 idle slots reset it). Aging runs against the STREAM position
 * (absolute block-row elapsed time), not candidate-job occurrences, so a
 * silent band still expires tracks; every contribute call also re-ages the
 * table against its own start position, so the TTL works even if the
 * receiver glue never calls wspr_agg_age_to() explicitly. Before the weights
 * are folded in, the slot's soft weights are correlation-checked against the
 * accumulated track (mean-removed Pearson, pure integer): a slot that does
 * not share the track's tone pattern is REJECTED and every track is left
 * untouched, so a payload change (A then B on one column) can cost a REJECT,
 * never a false mixed payload. RECOVERY IS BOUNDED, not TTL-bound: a track
 * that is stale past the TTL, or that rejects WSPR_AGG_TRACK_YIELD_REJECTS
 * (2) strong-grade candidates from DISTINCT transmissions, YIELDS its table
 * slot - the newcomer then opens a fresh cycle, and no weights ever cross
 * between the old and the new occupant. Repeated scan hypotheses of ONE
 * rejected transmission (onset within +-WSPR_AGG_REJECT_TX_TOL block-rows
 * and within +-WSPR_AGG_REJECT_TX_COL_TOL columns - drift-trade hypotheses
 * of one strong TX spread up to 4 columns, hence looser than the
 * WSPR_AGG_COL_MATCH merge tolerance) charge the counter at most once, so a
 * single strong interference burst cannot evict a healthy integration; weak
 * candidates never charge it at all. OPENING a new track additionally requires sync
 * excess >= WSPR_AGG_OPEN_EXCESS_MIN (10; see wspr_decode.c): the
 * incremental ring scan releases a flood of junk-grade excess-5..9
 * candidates, and a junk track shadows the real station's slots. KNOWN
 * LIMITATION: track and
 * dedup identity is the tone-0 column within +-2 bins - that is a frequency
 * match, NOT proof of station identity. Two stations within +-2 bins of one
 * another share a track, and a message change may then only surface as
 * gate failures (bounded by WSPR_AGG_MAX_FAILED_DECODES) rather than a
 * clean reset.
 */

/* Maximum integrated tracks (distinct stations accumulating at once). */
#define WSPR_AGG_TRACKS 4

/* Hard bound on the slots one track integrates. count is uint8 and MUST
 * saturate here instead of wrapping: a track that reaches this depth without
 * ever clearing the integrated-decode gates is reset by the contribution
 * step, so deeper integration is impossible by construction. Weights sum
 * with saturation; with the +-127 int8 per-slot clip, 8 slots sum to at most
 * +-1016, well inside int16. */
#define WSPR_AGG_MAX_COUNT 8

/* Integrated-decode attempt budget: after this many completions that FAIL a
 * gate (metric / agreement / unpack), the track resets. A stream of impostor
 * completions on one frequency can otherwise keep a garbage track alive
 * forever; the idle-age TTL below only bounds silent tracks. Attempts that
 * complete NO path (pure weak signal) are not charged: the count cap above
 * bounds those instead. */
#define WSPR_AGG_MAX_FAILED_DECODES 4

/* Track-table snapshot for tests/diagnostics. */
typedef struct {
    int16_t col0;  /* anchor tone-0 column                     */
    uint8_t count; /* slots accumulated                        */
    uint8_t age;   /* idle slots since the last contribution   */
    bool active;
} wspr_agg_track_info_t;

/* Release decoder returns NONE or REPORT. The latter two dispositions
 * describe single-slot preview and scanner dedup in experimental builds. */
enum {
    WSPR_CONTRIBUTION_NONE = 0,
    WSPR_CONTRIBUTION_REPORT = 1,
    WSPR_CONTRIBUTION_EARLY_REPORT = 2,
    WSPR_CONTRIBUTION_ALREADY_REPORTED = 3,
};

/*
 * Fold one slot's soft weights into the matching track and, at two or more
 * accumulated slots, attempt the integrated decode. `sym_weight` is the
 * 162-entry symbol-order int8 demodulation of `snap` (see
 * wspr_snapshot_demod_weights), `start_br` the candidate's absolute start
 * block-row. Only REPORT and EARLY_REPORT fill *out for the consumer.
 * ALREADY_REPORTED marks a decoded scanner window sharing evidence
 * with a prior early packet.
 */
int wspr_agg_track_contribute(const wspr_snapshot_t* snap,
                              const int8_t* sym_weight,
                              uint32_t start_br,
                              wspr_decode_result_t* out);

#if defined(WSPR_RX_EARLY_REPORTS)
/* Acknowledge a successfully enqueued EARLY_REPORT before the next
 * contribution. Without acknowledgement, a later matching report can publish. */
void wspr_agg_ack_early_report(void);
#endif

/* Copy up to max_tracks track-table entries to `out`; returns how many. */
int wspr_agg_tracks(wspr_agg_track_info_t* out, int max_tracks);

/*
 * Age the track table in SLOT units against the stream position `now_br` (the
 * newest absolute block-row the receiver has processed). The age of a track is
 * recomputed ABSOLUTELY from its own newest contribution, so the call is
 * idempotent: candidate jobs inside one slot leave the age at 0, and a silent
 * period ages every track one slot per 120 s even when no candidate fires.
 * Tracks idle for more than WSPR_AGG_TRACK_MAX_AGE slots are reset here. The
 * receiver calls it once per candidate job (or per completed block-row).
 */
void wspr_agg_age_to(uint32_t now_br);

/* Slots integrated behind the last successful report (the track resets on
 * success, so this is the only trace of the integration depth).
 * DIAGNOSTIC BUILD ONLY (see WSPR_DECODE_DIAG_COUNTERS). */
#if WSPR_DECODE_DIAG_COUNTERS
uint8_t wspr_agg_last_decode_slots(void);
#endif

/*
 * DIAGNOSTIC: reason code of the most recent wspr_agg_track_contribute call
 * (T01 observability split). Read immediately after the call; not synced with
 * anything else. Values 1, 2, 5, 6, 7, 8 keep the pre-T01 meaning; 3, 9 and 10
 * split reasons that were previously conflated, and 4 is NARROWED to the
 * message-consistency gate only. No value changes any accept/reject decision:
 * the gates themselves are unchanged and only the recorded reason differs.
 *
 * WSPR_AGG_RESULT_AWAIT_SLOT is an ACCEPTED contribution, not a rejection: the
 * slot folded into its track, but the track holds fewer than WSPR_AGG_MIN_COUNT
 * slots, so no decode is attempted yet. Classifying it as a reject (the
 * pre-T01 code did) understated every integration in progress.
 */
enum {
    WSPR_AGG_RESULT_NONE = 0,        /* no call / NULL argument               */
    WSPR_AGG_RESULT_AWAIT_SLOT = 1,  /* accepted, count < WSPR_AGG_MIN_COUNT:
                                        awaiting the next slot (NOT a reject) */
    WSPR_AGG_RESULT_DUPLICATE = 2,   /* same physical TX already contributed  */
    WSPR_AGG_RESULT_SLOT_WINDOW = 3, /* refused: gap is not a whole slot
                                        distance (k < 1 or |off| > TOL)      */
    WSPR_AGG_RESULT_CORRELATION = 4, /* refused: slot window passed but the
                                        message-consistency correlation failed */
    WSPR_AGG_RESULT_FEC_NO_PATH = 5, /* stack_decode() completed no path      */
    WSPR_AGG_RESULT_OPEN_FLOOR = 6,  /* junk-grade candidate below the open
                                        floor, nothing nearby                 */
    WSPR_AGG_RESULT_METRIC = 7,      /* completed path: metric below the
                                        report minimum                        */
    WSPR_AGG_RESULT_REPORT = 8,      /* integrated decode reported            */
    WSPR_AGG_RESULT_AGREEMENT = 9,   /* completed path: re-encode agreement
                                        below WSPR_MIN_AGREEMENT_PCT          */
    WSPR_AGG_RESULT_UNPACK = 10,     /* completed path: wspr_unpack_message
                                        rejected the payload                  */
};

/**
 * DIAGNOSTIC BUILD ONLY (WSPR_DECODE_DIAG_COUNTERS): reason code accessor.
 */
#if WSPR_DECODE_DIAG_COUNTERS
int wspr_agg_last_result_code(void);
#endif

/*
 * Real FEC-attempt counters (T01). The FEC attempt is a stack_decode() ENTRY,
 * not a demod/aggregate stage entry: candidate_decode() (the window and host
 * per-candidate path) and wspr_agg_track_contribute() (the integrated-slot
 * path) both call stack_decode(), and every such call is counted here.
 * wspr_fec_calls() counts entries, wspr_fec_no_path() counts the entries that
 * completed no path (the search exhausted its effort/bound with no complete
 * candidate). Both are cumulative and are zeroed by
 * wspr_decode_reset_aggregation() (session reset). They are NOT stage entries
 * and must never be used to derive a failure count by subtracting reports.
 * DIAGNOSTIC BUILD ONLY (WSPR_DECODE_DIAG_COUNTERS).
 */
#if WSPR_DECODE_DIAG_COUNTERS
uint32_t wspr_fec_calls(void);
uint32_t wspr_fec_no_path(void);
#endif

/*
 * Clear the track table. The receiver calls it once per app start so a
 * restart never inherits tracks from the previous run.
 */
void wspr_decode_reset_aggregation(void);

/*
 * ---- Ring scan / ring snapshot API (ring architecture, stage 3) ----
 * All ids are ABSOLUTE block-row ids into the producer's ring; block_rows is the ring
 * depth (352 for the OSR2 receiver). The scan and the snapshot reader walk the ring
 * wrap-safe: (br % block_rows + 2n) % block_rows, so a packet straddling the physical
 * seam and counters near the uint32 wrap both read the correct slots.
 *
 * OSR2-ONLY: the ring geometry (one start hypothesis per block-row, symbol n at
 * start + 2n, ring depth 352) exists for the OSR2 receiver, where a WSPR symbol
 * spans TWO block-rows. Under OSR1 the producer publishes one block-row per symbol
 * row, so a 176-row OSR1 ring cannot even contain a 323-block-row packet. The ring
 * functions therefore REJECT any block_rows smaller than a full packet plus the
 * deadline margin (return 0 / skip the session); the OSR1 host tests exercise only
 * the non-ring matrix/portapack paths.
 */

/* Reset a scan session: every start hypothesis from absolute id 0 on will be considered
 * once it becomes fully contained in the ring. */
void wspr_ring_scan_init(wspr_ring_scan_state_t* st);

/*
 * Incremental scan: process the next pending start hypothesis. Returns 1 and writes
 * *out_start_br (the absolute block-row id of symbol 0), *out_col (tone-0 column) and
 * *out_excess (mean sync-bin excess) when a candidate is RELEASED; the caller then
 * snapshots and decodes it, and MUST call wspr_ring_scan_mark_decoded() on a successful
 * DECODE. Returns 0 when the scan is caught up. Call once per completed block-row (or
 * whenever the producer moved): a released candidate is the best of its +-2 block-row
 * window, so a packet's candidates are compared the way the window decoder ranked its
 * pool and only the sharpest alignment reaches the decode.
 *
 * Skipped (not scanned): starts whose packet is truncated at the head of the stream,
 * starts past the overwrite deadline (counted in st->expired), and starts suppressed
 * by a decoded packet's anchor: within +-20 block-rows of an anchor ON ITS OWN
 * frequency column +-2 — the sync correlation plateau of one transmission spans
 * several symbols of start shift, and those ghost anchors must not re-decode (or
 * re-weigh) the same packet. A station on a DIFFERENT column is never suppressed by
 * another station's anchor (simultaneous starts are separate candidates). A start a
 * full slot later (352 block-rows = the 120 s WSPR repeat cadence) is the station's
 * NEXT transmission and scans normally, so multi-slot accumulation keeps integrating.
 *
 * BOUNDED PROGRESS: one call consumes at most `max_starts` start hypotheses (0
 * selects WSPR_RING_SCAN_MAX_STARTS_PER_POLL), then returns 0. A large backlog
 * (the producer ran ahead while a seconds-long FEC decode ran) can never wedge the
 * decode thread in one unbounded call; the caller re-polls whenever the producer
 * moves or the thread is woken, so the scan keeps making progress independent of
 * the FEC budget. `total_br` must be read FRESH immediately before each call; the
 * snapshot step re-verifies every row against the live cursor anyway. Each call
 * updates st->scan_lag / st->max_lag with the cursor distance of what it processed.
 *
 * MULTI-STATION (top-K): each start scans once and yields up to
 * 1 + WSPR_RING_MAX_SECONDARY excess-ranked candidates at distinct columns
 * (+-2-column NMS). The strongest runs the normal pending best-of-window
 * ranking; the others wait in the session's bounded secondary queue and are
 * released (in excess order, each on its own poll) before new starts are
 * scanned - so two stations sharing one onset both decode.
 *
 * RELEASE MARGIN: candidates are handed out no later than
 * block_rows - WSPR_RING_DEADLINE_ROWS - WSPR_RING_RELEASE_MARGIN_ROWS behind the
 * cursor (both the primary release and the secondary queue); anything later is
 * dropped and counted in st->expired. The snapshot keeps its harder deadline gate,
 * so the released candidate carries the margin as preemption allowance.
 *
 * SCAN STALENESS: the correlation itself reads the live matrix over the whole
 * packet span and can be preempted mid-scan; every scan is therefore bracketed by
 * LIVE cursor reads through `cursor_fn(cursor_ctx)` (NULL falls back to the one-shot
 * `total_br`, the same protocol as the snapshot's guarded reader). If the cursor at
 * scan end shows that any row of the span could have been overwritten or torn
 * (cursor - start_br > block_rows - 1), the whole scan result is DISCARDED before it
 * can become a pending/secondary candidate and st->stale_scans is counted. HONEST
 * LIMIT: this closes the read window, but the residual assumption is that a healthy
 * scan finishes well inside one block-row (~341 ms) so it is never falsely stale -
 * that wall time is NOT yet measured on ARM; do not declare the race closed until
 * it is.
 */
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
                        int8_t* out_drift);

/* Record a decoded packet at absolute start `start_br` (tone-0 column `col0`) as the
 * scan's dedup anchor and clear any held candidate. Call once per REPORTED decode. */
void wspr_ring_scan_mark_decoded(wspr_ring_scan_state_t* st, uint32_t start_br, int16_t col0);

/*
 * Sync-correlate ONE start hypothesis across all columns (full 162 symbols), the
 * single pass the incremental scan spends per start. Every (column, drift)
 * hypothesis' excess is already computed there, so the ranked top-K output is free:
 * results are the up to `max_results` strongest columns with excess >= min_excess
 * after +-2-column non-maximum suppression (one transmission's sync plateau spans
 * several columns; two STATIONS 3+ columns apart are kept side by side, excess
 * descending), written to out_cols/out_excess/out_drifts. Returns the count (>= 1
 * when the old single-best form returned 1; 0 when nothing clears the gate).
 * `total_br` bounds the packet: the caller must ensure
 * total_br > start_br + 2 * (162 - 1).
 */
/*
 * Sync-correlate ONE start hypothesis across all columns (full 162 symbols), the
 * single pass the incremental scan spends per start. Every (column, drift)
 * hypothesis' excess is already computed there, so the ranked top-K output is free:
 * results are the up to `max_results` strongest columns with excess >= min_excess
 * after +-2-column non-maximum suppression (one transmission's sync plateau spans
 * several columns; two STATIONS 3+ columns apart are kept side by side, excess
 * descending), written to out_cols/out_excess/out_drifts.
 *
 * Returns the count (>= 0), or -1 when the scan went STALE: the correlation reads
 * the live matrix for the whole packet span, so the cursor is loaded through
 * `cursor_fn(cursor_ctx)` (NULL -> the one-shot `total_br`) before and after the
 * pass; if at scan end any row of the span could have been overwritten or torn
 * (cursor - start_br > block_rows - 1, the same publication-protocol guard as the
 * snapshot's guarded copies), the result describes rewritten bytes and the caller
 * must discard it (counted in st->stale_scans by the poll). HONEST LIMIT: the
 * residual assumption is a scan wall time well inside one block-row (~341 ms),
 * unmeasured on ARM so far.
 * `total_br` bounds the packet: the caller must ensure
 * total_br > start_br + 2 * (162 - 1).
 */
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
                         int8_t* out_drifts);

/*
 * Freeze ONE candidate out of the RING buffer, the wrap-safe counterpart of
 * wspr_snapshot_candidate(): the packet's symbol n reads physical slot
 * (start_br % block_rows + 2n) % block_rows, whatever the ring seam and the uint32
 * counter position. Drift refinement over -3..+3 runs here on the same excess scale as
 * the window decoder's stage 2.
 *
 * GUARDED READER: `total_br` is only the caller's last-known value; the LIVE cursor
 * comes through `cursor_fn(cursor_ctx)` (wspr_ring_cursor_fn, see the protocol note
 * near the top of this header). Pass NULL to fall back to the one-shot `total_br`.
 * Before the first byte is read the packet is checked against the REFRESHED cursor
 * for the overwrite deadline, and EVERY symbol row is copied in one guarded step:
 * refresh cursor -> guard (row published AND its slot cannot have been rewritten) ->
 * copy the row into a local scratch -> refresh cursor again -> guard must still hold,
 * otherwise the copy raced the writer and the whole snapshot is rejected. On the
 * single-core M4 this closes the only torn-read window a preempting producer can
 * open (see the protocol note for exactly what is and is not guaranteed).
 *
 * Returns 1 on success. Returns 0 on bad arguments/geometry or when the packet
 * expired or raced the writer; `*out_fail` (if non-NULL) then carries one of the
 * WSPR_SNAP_* reason codes so the caller can count expired vs overwritten drops.
 */
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
                                 int* out_fail);

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_DECODE_H_
