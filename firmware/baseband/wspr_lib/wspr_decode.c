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
 * 5. Multi-slot weight accumulation: every agreement-passing, unpack-valid
 *    completion folds its soft weights into the payload-keyed cluster of its
 *    message (a station repeats the same message across 2-minute slots, and
 *    the weights are transmission-aligned in symbol order), and clusters
 *    holding two or more windows decode on their accumulated weights through
 *    the same gates - the sensitivity extension that reaches stations below
 *    the per-window threshold.
 */

// 96 nodes x 14 bytes. The bounded frontier shares the 88 KiB M4 RAM with
// the dual-bank matrix, decoder stack, processor, and both DMA engines. A full
// pool evicts its weakest path; keep the weak-signal host cases when tuning it.
#define WSPR_STACK_NODES 96
#define WSPR_STACK_EFFORT 40000
#define WSPR_METRIC_LIMIT 250
// Crowded-band budget: eight stations in one real-ether recording out-compete
// each other for six slots, so the weaker true peaks never reach the decode.
#define WSPR_MAX_CANDIDATES 12
#define WSPR_DRIFT_MIN (-3)
#define WSPR_DRIFT_MAX 3

// Candidates whose sync bins barely rise above the pair-bin noise mean are
// noise ridges, not transmissions: decoding them only manufactures impostor
// messages for the report gate to fight. The mean pair-bin byte runs a couple
// of units below the tone bins of a real signal; anything closer than this to
// the mean is skipped before the decoder runs on it.
#define WSPR_CANDIDATE_MIN_EXCESS 6

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
    uint8_t path[WSPR_SOURCE_BYTES];  // source bits, MSB-first
} wspr_stack_node_t;

// The node pool and the decoded weights live in .bss: the decode thread's
// 4 KiB stack cannot hold them.
static wspr_stack_node_t node_pool[WSPR_STACK_NODES];
static int node_count;
static int8_t bit_weight[WSPR_CODED_BITS];
static int8_t sym_weight[WSPR_CODED_BITS];
// Best remaining metric gain per depth, for the branch-and-bound prune.
static int16_t suffix_gain[WSPR_SOURCE_BITS + 1];

/*
 * Multi-slot weight accumulation - the sensitivity extension.
 *
 * A WSPR station repeats the SAME message in successive 2-minute slots
 * (typical duty 20-100%), and every slot's soft demodulation produces
 * sym_weight[] in TRANSMISSION-aligned symbol order (symbol 0 is the start of
 * the transmission, wherever it sits in the capture window: the demod walks
 * best_row[k] + n). Summing N windows' weights adds the signal N times while
 * the independent noise grows only as sqrt(N), so the accumulated decode
 * gains ~sqrt(N) of SNR: a station each of whose windows sits below the
 * per-window acceptance gates can still clear them once enough slots have
 * been integrated.
 *
 * The sub-row PHASE may differ between windows (a different bank may carry
 * the aligned decode, the capture start shifts); that only moves the
 * transmission inside the window, never the symbol indexing - symbol 0 is the
 * transmission start in every window, so the weights sum coherently in symbol
 * index regardless.
 *
 * WHICH completions feed a cluster: every stack-decode completion that passes
 * the re-encode agreement gate AND unpacks into a valid standard message
 * contributes its weights - including completions the metric gate is about to
 * reject. That is the whole point: a station below the per-window threshold
 * still completes at its true alignment with high agreement in most windows
 * (measured at -26 dB: the true payload completes with agreement 97-99 and a
 * path metric of 13-19 against the report gate of 20), and integrating those
 * windows is what lifts it over the gate. The reporting path below keeps the
 * full gate set, so contributing is strictly weaker than reporting. The known
 * limitation: a completion can still be an impostor (WSPR has no CRC). Its
 * weights then do not carry the true transmission's symbol pattern - but
 * clusters are keyed by PAYLOAD (see below), so a poisoning impostor must
 * complete the SAME wrong message twice, which independent noise essentially
 * never does; a noise window contributes nothing that survives.
 *
 * Cluster identity is the 50 payload bits, not the frequency: the same
 * station repeats the same message every slot, while impostor completions are
 * independent random payloads that never repeat. Two completions of one
 * message can still differ in the 31 decoder flush bits (noise moves them;
 * unpacking only reads the payload), so the key masks everything past the
 * payload. Payload identity also survives the few-Hz frequency wander that a
 * +-4 Hz match would clip, and it cannot merge two stations the way a
 * frequency match can.
 *
 * A cluster decodes on the SUM of its accumulated weights (clipped into the
 * int8 the stack decoder reads), not the window average. The sum is the honest
 * evidence accumulator - every window's weight keeps its full magnitude, so
 * coherent symbols grow with N - while an average rounds the signal lean back
 * into the integer grid and, with the stack decoder's r_bias = max(1,
 * mean|w|/2) floor, collapses the path metric below the report gate as count
 * grows (measured: two slots decode at metric 24, adding a third coherent
 * slot drops the averaged decode to 6). On the sum scale the same gates
 * separate harder, not softer: the true path climbs to hundreds while noise
 * completions sink below zero. Decoded clusters are NOT reset - a station
 * keeps transmitting, and every further window sharpens the integration.
 */
#define WSPR_AGG_CLUSTERS 8
#define WSPR_AGG_MIN_COUNT 2   // windows accumulated before a decode is attempted
#define WSPR_AGG_MAX_COUNT 64  // enough integration; drop the slot afterwards
#define WSPR_AGG_MAX_AGE 15    // windows since the last match before dropping

// The payload key: 50 payload bits = bytes 0..5 plus the top 2 bits of byte 6.
#define WSPR_AGG_KEY_BYTES 7
#define WSPR_AGG_KEY_LAST_MASK 0xC0u

typedef struct {
    int16_t weight[WSPR_CODED_BITS];      // accumulated soft weights, symbol order
    uint8_t payload[WSPR_AGG_KEY_BYTES];  // the repeated message's payload bits
    int16_t freq_hz;                      // tone 0 of the first contributing decode
    int16_t last_row_offset;              // row_offset of the newest contributing
                                          // decode, so the aggregated result keeps
                                          // feeding the app's slot tracker
    int8_t last_snr_db;                   // newest contributing decode's SNR readout
    int8_t last_sync_score;               // newest contributing decode's sync excess
    uint8_t count;                        // windows accumulated
    uint8_t age;                          // windows since the last match
    uint8_t bank;                         // majority bank of the accumulated windows
    bool active;
} wspr_agg_cluster_t;

static wspr_agg_cluster_t agg_clusters[WSPR_AGG_CLUSTERS];

void wspr_decode_reset_aggregation(void) {
    memset(agg_clusters, 0, sizeof(agg_clusters));
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
        const uint32_t b = bit_at(path, i);
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
        if (node_pool[i].metric < node_pool[mi].metric) mi = i;
    }
    if (node_pool[mi].metric >= metric) return;
    node_pool[mi].metric = metric;
    node_pool[mi].depth = depth;
    memcpy(node_pool[mi].path, path, WSPR_SOURCE_BYTES);
    int i = mi;
    while (i > 0) {
        const int parent = (i - 1) / 2;
        if (node_pool[parent].metric >= node_pool[i].metric) break;
        const wspr_stack_node_t tmp = node_pool[parent];
        node_pool[parent] = node_pool[i];
        node_pool[i] = tmp;
        i = parent;
    }
    while (true) {
        int best = i;
        const int l = 2 * i + 1;
        const int r = 2 * i + 2;
        if (l < node_count && node_pool[l].metric > node_pool[best].metric) best = l;
        if (r < node_count && node_pool[r].metric > node_pool[best].metric) best = r;
        if (best == i) break;
        const wspr_stack_node_t tmp = node_pool[best];
        node_pool[best] = node_pool[i];
        node_pool[i] = tmp;
        i = best;
    }
}

static void heap_push(int16_t metric, uint8_t depth, const uint8_t* path) {
    if (node_count >= WSPR_STACK_NODES) {
        heap_replace_min(metric, depth, path);
        return;
    }
    int i = node_count++;
    node_pool[i].metric = metric;
    node_pool[i].depth = depth;
    memcpy(node_pool[i].path, path, WSPR_SOURCE_BYTES);
    while (i > 0) {
        const int parent = (i - 1) / 2;
        if (node_pool[parent].metric >= node_pool[i].metric) break;
        const wspr_stack_node_t tmp = node_pool[parent];
        node_pool[parent] = node_pool[i];
        node_pool[i] = tmp;
        i = parent;
    }
}

static void heap_pop(wspr_stack_node_t* out) {
    *out = node_pool[0];
    node_count--;
    if (node_count == 0) return;
    node_pool[0] = node_pool[node_count];
    int i = 0;
    while (true) {
        int best = i;
        const int l = 2 * i + 1;
        const int r = 2 * i + 2;
        if (l < node_count && node_pool[l].metric > node_pool[best].metric) best = l;
        if (r < node_count && node_pool[r].metric > node_pool[best].metric) best = r;
        if (best == i) break;
        const wspr_stack_node_t tmp = node_pool[best];
        node_pool[best] = node_pool[i];
        node_pool[i] = tmp;
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
    uint8_t path[WSPR_SOURCE_BYTES];
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
            memcpy(out_bits, node.path, WSPR_SOURCE_BYTES);
            if (out_metric) *out_metric = node.metric;
            continue;
        }
        effort++;
        if (node.metric < -WSPR_METRIC_LIMIT) continue;

        uint32_t reg0, reg1;
        path_registers(node.path, node.depth, &reg0, &reg1);
        const int child_bound = have_best ? best_complete - suffix_gain[node.depth + 1] : (-32768);
        for (uint8_t b = 0; b < 2; b++) {
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
            uint8_t child_path[WSPR_SOURCE_BYTES];
            memcpy(child_path, node.path, WSPR_SOURCE_BYTES);
            set_bit(child_path, node.depth, b);
            heap_push((int16_t)m, (uint8_t)(node.depth + 1), child_path);
        }
    }
    return have_best;
}

/*
 * Sync score of one alignment: sum of the better bin of every symbol's
 * sync-selected pair. The transmitted tone of symbol n is
 * col0 + sync[n] + 2*data[n] (wspr_merge_sync), so the sync bit picks the
 * pair {col0+sync[n], col0+sync[n]+2} and the data bit picks which bin of
 * the pair carries the tone. base/stride implement the time_osr view:
 * address of (row, col) = row * stride + base + col, with base = bank *
 * num_bins and stride = time_osr * num_bins. out_mean (optional) receives
 * the mean of both bins of every pair, the local noise reference the raw
 * score has to be read against.
 */
static int32_t sync_score(const uint8_t* mag,
                          int num_bins,
                          int base,
                          int stride,
                          int row0,
                          int col0,
                          int drift,
                          int32_t* out_mean) {
    int32_t score = 0;
    int32_t sum_pair = 0;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int row = row0 + n;
        int col = col0;
        if (drift != 0) {
            col += (drift * n) / (WSPR_CODED_BITS - 1);
        }
        col += wspr_sync_vector[n];
        if (col < 0 || col + 2 >= num_bins) return -1;
        const uint8_t e0 = mag[row * stride + base + col];
        const uint8_t e1 = mag[row * stride + base + col + 2];
        score += (e0 > e1) ? e0 : e1;
        sum_pair += e0 + e1;
    }
    if (out_mean) *out_mean = sum_pair / (2 * WSPR_CODED_BITS);
    return score;
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

// Re-encode agreement, the impostor gate: regenerate the channel symbols from
// the decoded payload and count hard disagreements with the received hard
// decisions (static sym_weight, symbol order). WSPR has no CRC, so payload
// validity alone admits about one impostor per seven completions; a true
// decode agrees with nearly every symbol while a noise completion disagrees
// on about half (wsprd's count_hard_errors, JT65's deep-search acceptance
// rule).
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

/*
 * Fold one completed per-window decode into the cluster table. The decode's
 * sym_weight is transmission-aligned in symbol order, so it adds onto the
 * cluster's weights coherently (see the block comment on the cluster table).
 * source carries the completed payload bits; the other fields describe the
 * contributing decode for the aggregated result's display/tracker fields.
 */
static void agg_accumulate(const uint8_t* source,
                           const wspr_decode_result_t* contribution,
                           uint8_t* matched_mask) {
    wspr_agg_cluster_t* c = NULL;

    // Match by payload: the same station repeats the same message every slot.
    for (int i = 0; i < WSPR_AGG_CLUSTERS; i++) {
        if (agg_clusters[i].active &&
            memcmp(agg_clusters[i].payload, source, WSPR_AGG_KEY_BYTES) == 0) {
            c = &agg_clusters[i];
            break;
        }
    }
    if (c == NULL) {
        // Free slot first, else evict the cluster with the fewest accumulated
        // windows (age breaks ties): an integrated cluster represents real
        // evidence and must not be flushed by the churn of one-window impostor
        // completions.
        int best = -1;
        for (int i = 0; i < WSPR_AGG_CLUSTERS; i++) {
            if (!agg_clusters[i].active) {
                best = i;
                break;
            }
            if (best < 0 || agg_clusters[i].count < agg_clusters[best].count ||
                (agg_clusters[i].count == agg_clusters[best].count &&
                 agg_clusters[i].age > agg_clusters[best].age)) {
                best = i;
            }
        }
        c = &agg_clusters[best];
        memset(c, 0, sizeof(*c));
        memcpy(c->payload, source, WSPR_AGG_KEY_BYTES);
        // Mask the flush bits out of the key byte: two completions of one
        // message differ there (noise moves them), the payload does not.
        c->payload[WSPR_AGG_KEY_BYTES - 1] &= WSPR_AGG_KEY_LAST_MASK;
        c->freq_hz = contribution->freq_hz;
        c->bank = (uint8_t)contribution->bank;
    }

    // One slot per window per cluster: the same payload can complete at more
    // than one candidate of the same window, and counting it twice would
    // double that window's signal.
    const uint8_t bit = (uint8_t)(1u << (c - agg_clusters));
    if (*matched_mask & bit) return;

    c->active = true;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        c->weight[n] += sym_weight[n];
    }
    // Running majority of the contributing banks: with banks in {0,1} the
    // rounded running mean is the majority.
    c->bank = (uint8_t)((c->bank * c->count + contribution->bank) / (c->count + 1));
    c->count++;
    c->age = 0;
    c->last_row_offset = contribution->row_offset;
    c->last_snr_db = contribution->snr_db;
    c->last_sync_score = contribution->sync_score;
    *matched_mask |= bit;
}

/*
 * Attempt one cluster decode on the SUM of its accumulated weights, clipped
 * into the int8 the stack decoder reads, through the same stack decode and
 * acceptance gates as a per-window decode. Returns true and fills *out
 * (agg_windows NOT set - the caller takes it from the cluster) when the
 * accumulated weights decode.
 */
static bool agg_decode_cluster(const wspr_agg_cluster_t* c, wspr_decode_result_t* out) {
    // The raw sum fits int8 while count <= 21 (weights clip at +-6 per
    // window); past that only repeatedly strong symbols clip, which costs the
    // tail of the soft scale but keeps every sign. The gates read the sum
    // scale (see the cluster table's block comment).
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        int32_t w = c->weight[n];
        if (w > 127) w = 127;
        if (w < -127) w = -127;
        sym_weight[n] = (int8_t)w;
    }
    deinterleave_weights(sym_weight, bit_weight);

    uint8_t source[WSPR_SOURCE_BYTES];
    int16_t metric = 0;
    if (!stack_decode(bit_weight, source, &metric)) return false;
    if (metric < WSPR_REPORT_METRIC_MIN) return false;
    const int agreement = reencode_agreement(source);
    if (agreement < WSPR_MIN_AGREEMENT_PCT) return false;

    wspr_message_t message;
    if (!wspr_unpack_message(source, &message)) return false;

    memset(out, 0, sizeof(*out));
    memcpy(out->callsign, message.callsign, sizeof(message.callsign));
    memcpy(out->grid, message.grid, sizeof(message.grid));
    out->dbm = message.dbm;
    // Effective SNR of the integrated decode: the newest contributing
    // window's readout plus the coherent-integration gain 10*log10(count)
    // dB, in integer steps of 3 dB per doubling.
    int shift = 0;
    for (uint32_t v = c->count; v > 1; v >>= 1) shift++;
    int snr = c->last_snr_db + 3 * shift;
    if (snr > 127) snr = 127;
    out->snr_db = (int8_t)snr;
    out->freq_hz = c->freq_hz;
    // The integration spans whole windows, so a per-window drift figure is
    // meaningless for the accumulated decode.
    out->drift_bins = 0;
    // The tracker-relevant fields ride from the newest contributing decode:
    // the app steers its capture phase on row_offset and shows the sync
    // excess, and the accumulated weights carry no alignment scan of their own.
    out->row_offset = c->last_row_offset;
    out->sync_score = c->last_sync_score;
    out->metric = metric;
    out->bank = (int8_t)c->bank;
    out->agreement_pct = (int8_t)agreement;
    return true;
}

int wspr_decode_window(const uint8_t* mag,
                       int num_rows,
                       int num_bins,
                       int bin0_hz,
                       wspr_decode_result_t* results,
                       int max_results,
                       int time_osr) {
    if (mag == NULL || results == NULL || max_results <= 0) return 0;
    if (num_rows < WSPR_NUM_SYMBOLS || num_bins < 8) return 0;
    if (time_osr != 1 && time_osr != 2) return 0;

    // Per-bank geometry of the interleaved matrix: the address of (row, col)
    // in bank `bank` is row * stride + base + col. time_osr = 1 degenerates
    // to the plain single-grid matrix (base 0, stride num_bins).
    const int stride = time_osr * num_bins;

    // Accepted decodes from every bank land here first, get ranked, and only
    // then go through the duplicate/frequency suppression. The two banks see
    // the same transmission, and the misaligned bank decodes first with
    // smeared symbols - a structurally valid impostor at the same audio
    // frequency (no CRC protects the payload). Alignment sharpness separates
    // them: the aligned bank's mean sync-bin excess runs about twice the
    // smeared one's, so the pool is ranked on it before one decode per
    // station/frequency is kept. Aggregated cluster decodes join the same
    // pool after the bank loop, so the duplicate suppression below sees one
    // list: an aggregated decode of a payload already reported per-window
    // outranks it.
    wspr_decode_result_t merged[WSPR_MAX_CANDIDATES * 2 + WSPR_AGG_CLUSTERS];
    int n_merged = 0;

    // Clusters matched (or created) by THIS window's accepted decodes; the
    // aging pass at the end of the window only increments the others.
    uint8_t agg_matched = 0;

    // Stage 1 + stage 2 run per sub-row bank: each bank is a full view of the
    // window on its own row grid, with its own candidate list.
    int count = 0;
    for (int bank = 0; bank < time_osr; bank++) {
        const int base = bank * num_bins;

        // Stage 1: coarse scan, no drift. Track the best few, with a simple
        // non-maximum suppression around each accepted peak. Candidates rank
        // by EXCESS - the sync score per symbol over the candidate's own
        // pair-bin mean - not by the raw score: the raw score carries the
        // local noise background, and on a crowded band a hot patch of
        // background (a strong neighbour's Goertzel skirts, say) would
        // otherwise outrank a cooler-background station with the same signal
        // excess and eat the candidate budget with its ridge.
        int best_row[WSPR_MAX_CANDIDATES];
        int best_col[WSPR_MAX_CANDIDATES];
        int32_t best_excess[WSPR_MAX_CANDIDATES];  // ranking key
        int n_cand = 0;
        const int t_max = num_rows - WSPR_NUM_SYMBOLS;

        for (int t = 0; t <= t_max; t++) {
            for (int c = 0; c + 3 < num_bins; c++) {
                int32_t mean = 0;
                const int32_t score = sync_score(mag, num_bins, base, stride, t, c, 0, &mean);
                if (score < 0) continue;
                const int32_t excess = score / WSPR_CODED_BITS - mean;
                bool suppressed = false;
                for (int k = 0; k < n_cand; k++) {
                    if (t >= best_row[k] - 1 && t <= best_row[k] + 1 &&
                        c >= best_col[k] - 1 && c <= best_col[k] + 1 &&
                        excess <= best_excess[k]) {
                        suppressed = true;
                        break;
                    }
                }
                if (suppressed) continue;
                // Insert into the descending top list. When the list is full the
                // last entry is the insertion point; otherwise append and bubble
                // the new score up.
                int pos;
                if (n_cand == WSPR_MAX_CANDIDATES) {
                    if (excess <= best_excess[WSPR_MAX_CANDIDATES - 1]) continue;
                    pos = WSPR_MAX_CANDIDATES - 1;
                } else {
                    pos = n_cand++;
                }
                while (pos > 0 && best_excess[pos - 1] < excess) {
                    best_row[pos] = best_row[pos - 1];
                    best_col[pos] = best_col[pos - 1];
                    best_excess[pos] = best_excess[pos - 1];
                    pos--;
                }
                best_row[pos] = t;
                best_col[pos] = c;
                best_excess[pos] = excess;
            }
        }

        // Stage 2: drift refinement, soft demodulation and decoding, best first.
        for (int k = 0; k < n_cand && n_merged < (int)(sizeof(merged) / sizeof(merged[0])); k++) {
            // Drift refinement on the same excess scale as the ranking: a raw
            // score comparison would walk the alignment toward whatever drift
            // lands its pair bins on the hottest patch of background, and the
            // demodulation that follows reads a smeared signal.
            int best_drift = 0;
            int32_t drift_excess = best_excess[k];
            for (int d = WSPR_DRIFT_MIN; d <= WSPR_DRIFT_MAX; d++) {
                if (d == 0) continue;
                int32_t dmean = 0;
                const int32_t s =
                    sync_score(mag, num_bins, base, stride, best_row[k], best_col[k], d, &dmean);
                if (s < 0) continue;
                const int32_t e = s / WSPR_CODED_BITS - dmean;
                if (e > drift_excess) {
                    drift_excess = e;
                    best_drift = d;
                }
            }

            // Soft demodulation: data bit n is the energy difference between the
            // two bins of symbol n's sync-selected pair, col0+sync[n] (bit 0) and
            // col0+sync[n]+2 (bit 1). The pair bins double as the noise
            // reference: their standard deviation sets the unit of the weights,
            // so a tone a few bytes above the noise gives a weight of a few
            // units regardless of the absolute byte scale.
            int pair_cols[WSPR_CODED_BITS];
            int32_t sum_pair = 0;
            int32_t sum_sq = 0;
            bool in_range = true;
            for (int n = 0; n < WSPR_CODED_BITS; n++) {
                int col = best_col[k];
                if (best_drift != 0) {
                    col += (best_drift * n) / (WSPR_CODED_BITS - 1);
                }
                col += wspr_sync_vector[n];
                if (col < 0 || col + 2 >= num_bins) {
                    in_range = false;
                    break;
                }
                pair_cols[n] = col;
                const int row = best_row[k] + n;
                const int e0 = mag[row * stride + base + col];
                const int e1 = mag[row * stride + base + col + 2];
                sum_pair += e0 + e1;
                sum_sq += e0 * e0 + e1 * e1;
            }
            // A drift walk that leaves the matrix disqualifies the alignment.
            if (!in_range) continue;

            // Mean and standard deviation over both bins of every pair.
            const int count_pair = 2 * WSPR_CODED_BITS;
            const int mean = (int)(sum_pair / count_pair);
            const int32_t variance = sum_sq / count_pair - mean * mean;
            const int sigma = variance > 1 ? (int)sqrt_i32(variance) : 1;

            // Pre-gate: this candidate's sync bins must rise clearly above its own
            // pair-bin noise mean before the decoder spends effort on it.
            if (drift_excess < WSPR_CANDIDATE_MIN_EXCESS) continue;

            for (int n = 0; n < WSPR_CODED_BITS; n++) {
                const int row = best_row[k] + n;
                const int e0 = mag[row * stride + base + pair_cols[n]];
                const int e1 = mag[row * stride + base + pair_cols[n] + 2];
                // Difference normalized by sigma, rounded to nearest, clipped.
                int w = e1 - e0;
                w = (w >= 0) ? ((2 * w + sigma) / (2 * sigma))
                             : -((2 * -w + sigma) / (2 * sigma));
                if (w > 6) w = 6;
                if (w < -6) w = -6;
                sym_weight[n] = (int8_t)w;
            }

            // The channel symbols carry interleaved coded bits: undo the
            // bit-reversal interleave so the stack decoder sees them in the
            // encoder's natural (poly0, poly1) order.
            deinterleave_weights(sym_weight, bit_weight);

            uint8_t source[WSPR_SOURCE_BYTES];
            int16_t metric = 0;
            if (!stack_decode(bit_weight, source, &metric)) continue;

            // Re-encode agreement, the impostor gate: regenerate the channel
            // symbols from the decoded payload and count hard disagreements
            // with the received hard decisions (see reencode_agreement). Read
            // BEFORE the metric gate: every completion that gets this far is a
            // candidate contribution for the multi-slot accumulation below,
            // and the reporting path then applies the metric gate on top.
            const int agreement = reencode_agreement(source);
            if (agreement < WSPR_MIN_AGREEMENT_PCT) continue;

            wspr_message_t message;
            if (!wspr_unpack_message(source, &message)) continue;

            // Audio frequency of this candidate's tone 0.
            const int16_t cand_freq =
                (int16_t)(bin0_hz + (best_col[k] * WSPR_SAMPLE_RATE) / WSPR_SAMPLES_PER_SYMBOL);
            // Rough SNR for display: mean tone-bin excess over the pair mean, in
            // 0.5 dB bytes converted to dB.
            int32_t signal_sum = 0;
            for (int n = 0; n < WSPR_CODED_BITS; n++) {
                const int row = best_row[k] + n;
                // The winning bin of symbol n's pair: +2 for data bit 1.
                const int col = pair_cols[n] + (sym_weight[n] >= 0 ? 2 : 0);
                signal_sum += mag[row * stride + base + col];
            }
            const int8_t snr_est = (int8_t)((signal_sum / WSPR_CODED_BITS - mean) / 2);

            // Every agreement-passing, unpack-valid completion feeds its
            // payload's cluster - including the completions the metric gate
            // rejects below. A station below the per-window threshold still
            // completes at its true alignment with high agreement in most
            // windows, and integrating those windows is what the accumulation
            // is for (see the cluster table's block comment). sym_weight still
            // holds this candidate's demodulation here.
            {
                wspr_decode_result_t contribution;
                memset(&contribution, 0, sizeof(contribution));
                contribution.freq_hz = cand_freq;
                contribution.row_offset = (int16_t)best_row[k];
                contribution.snr_db = snr_est;
                contribution.sync_score = (int16_t)drift_excess;
                contribution.bank = (int8_t)bank;
                agg_accumulate(source, &contribution, &agg_matched);
            }

            // A completed path on noise is not a report (see WSPR_REPORT_METRIC_MIN).
            if (metric < WSPR_REPORT_METRIC_MIN) continue;

            // Park the decode in the merge pool; ranking and duplicate
            // suppression across candidates and banks happen once every bank
            // has decoded (see merged above).
            wspr_decode_result_t* out = &merged[n_merged++];
            memset(out, 0, sizeof(*out));
            memcpy(out->callsign, message.callsign, sizeof(message.callsign));
            memcpy(out->grid, message.grid, sizeof(message.grid));
            out->dbm = message.dbm;
            out->snr_db = snr_est;
            out->freq_hz = cand_freq;
            out->drift_bins = (int16_t)best_drift;
            // Display metric: mean sync-bin excess over the pair-bin noise mean,
            // in 0.5 dB bytes, of the drift-refined alignment. Noise windows
            // score a couple of units, a real transmission several times that.
            out->sync_score = (int16_t)drift_excess;
            // Where in the window the transmission started, in SYMBOL rows of
            // this bank's grid, for the slot tracker. The bank is reported
            // alongside; its sub-row phase never leaves the decoder.
            out->row_offset = (int16_t)best_row[k];
            out->bank = (int8_t)bank;
            // Final path metric: the decode-acceptance gate read this already, and
            // the UI/PR can use it to judge decode quality.
            out->metric = metric;
            out->agreement_pct = (int8_t)agreement;
        }
    }

    // Multi-slot accumulation: every cluster holding at least two integrated
    // windows gets a decode attempt on its accumulated weights, through the
    // same stack decode and acceptance gates as a per-window decode. Decoded
    // clusters are not reset - they keep integrating, and the deduplication
    // below plus the app's two-slot confirmation handle repeat reporting.
    for (int i = 0; i < WSPR_AGG_CLUSTERS && n_merged < (int)(sizeof(merged) / sizeof(merged[0])); i++) {
        const wspr_agg_cluster_t* c = &agg_clusters[i];
        if (!c->active || c->count < WSPR_AGG_MIN_COUNT) continue;
        wspr_decode_result_t agg;
        if (!agg_decode_cluster(c, &agg)) continue;
        agg.agg_windows = (int8_t)c->count;
        merged[n_merged++] = agg;
    }

    // Age the clusters: windows since the last match; a station that stopped
    // transmitting frees its slot, and a fully integrated cluster (64 windows,
    // some two hours of repeats) does too.
    for (int i = 0; i < WSPR_AGG_CLUSTERS; i++) {
        wspr_agg_cluster_t* c = &agg_clusters[i];
        if (!c->active) continue;
        if (c->count >= WSPR_AGG_MAX_COUNT) {
            c->active = false;
            continue;
        }
        if (!(agg_matched & (1u << i))) {
            c->age++;
            if (c->age > WSPR_AGG_MAX_AGE) c->active = false;
        }
    }

    // Rank the merge pool: aggregated decodes (agg_windows >= 2) first - their
    // weights integrate several windows, so they outrank any per-window decode
    // of the same station - then by alignment sharpness (the mean sync-bin
    // excess stored in sync_score): the aligned bank of a transmission
    // outranks its smeared twin. The duplicate pass below keeps the first
    // entry, so it keeps the aggregated decode over the per-window one and the
    // true decode over the impostor the misaligned bank produced alongside it.
    // Stable insertion sort over a pool of at most
    // 2 * WSPR_MAX_CANDIDATES + WSPR_AGG_CLUSTERS.
    for (int i = 1; i < n_merged; i++) {
        const wspr_decode_result_t key = merged[i];
        const bool key_agg = key.agg_windows >= 2;
        int j = i - 1;
        while (j >= 0) {
            const bool cur_agg = merged[j].agg_windows >= 2;
            if (cur_agg > key_agg) break;  // aggregated entries never sink
            if (cur_agg == key_agg && merged[j].sync_score >= key.sync_score) break;
            merged[j + 1] = merged[j];
            j--;
        }
        merged[j + 1] = key;
    }

    // Duplicate suppression across candidates and banks: an identical payload
    // is the same station found twice, and so is a different payload at the
    // same audio frequency - the no-CRC code lets one transmission decode
    // into two valid-shaped impostors, and wsprnet has no room for either the
    // second copy or the misprint. Keep the first, now the sharpest aligned.
    for (int i = 0; i < n_merged && count < max_results; i++) {
        bool duplicate = false;
        for (int j = 0; j < count; j++) {
            if (strcmp(results[j].callsign, merged[i].callsign) == 0 &&
                strcmp(results[j].grid, merged[i].grid) == 0 &&
                results[j].dbm == merged[i].dbm) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;
        results[count++] = merged[i];
    }
    return count;
}
