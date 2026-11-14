/*
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

#include "lrpt_demod.hpp"

#include <cmath>
#include <cstring>

namespace lrpt {

namespace {

constexpr uint8_t POLY_A = 0x4F;
constexpr uint8_t POLY_B = 0x6D;

inline uint8_t parity(uint32_t x) {
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return x & 1;
}

// code pair (G1 << 1 | G2) of the encoder register 2j, j = 0..31
constexpr uint8_t code_pair(uint32_t r) {
    uint32_t a = r & POLY_A;
    uint32_t b = r & POLY_B;
    a ^= a >> 4;
    a ^= a >> 2;
    a ^= a >> 1;
    b ^= b >> 4;
    b ^= b >> 2;
    b ^= b >> 1;
    return ((a & 1) << 1) | (b & 1);
}

struct CodeTable {
    uint8_t v[32];
    constexpr CodeTable()
        : v{} {
        for (uint32_t j = 0; j < 32; j++) v[j] = code_pair(j * 2);
    }
};
constexpr CodeTable code_table{};

inline int8_t clamp_soft(float v) {
    if (v > 127.0f) return 127;
    if (v < -127.0f) return -127;
    return (int8_t)v;
}

inline Cf mul(const Cf& a, const Cf& b) {
    return {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
}

// Timing loop: Gardner at 2 samples a symbol, unit power.
constexpr float TIMING_KP = 0.01f;
constexpr float TIMING_KI = 0.00003f;
constexpr float TIMING_MAX_ADJUST = 0.002f;  // 2000 ppm

// Carrier loop, decision directed, per symbol: wide to pull in, narrow once the code is locked
// so that it does not slip.
constexpr float CARRIER_ALPHA_ACQUIRE = 0.02f;
constexpr float CARRIER_BETA_ACQUIRE = 0.00005f;
constexpr float CARRIER_ALPHA = 0.01f;
constexpr float CARRIER_BETA = 0.000025f;
constexpr float OMEGA_MAX = 0.2f;  // rad/sample at 4 samples a symbol, 9 or 10 kHz

// FLL, until the code locks: the phase of the lag 1 autocorrelation ahead of the matched
// filter, over 32768 samples. The spectrum is symmetric, so it needs neither the timing nor the
// carrier phase. Big steps far out, small ones close in, where the estimates are mostly noise
// and the carrier loop pulls in the rest.
constexpr uint16_t FLL_SAMPLES = 32768;
constexpr float FLL_GAIN_FAR = 0.6f;
constexpr float FLL_GAIN_NEAR = 0.15f;

constexpr float SOFT_SCALE = 70.0f;
constexpr uint16_t BER_LOCK_PERMILLE = 120;

// 80k, the softs as the deinterleaver keeps them. 2 bits: sign and strong; 4 bits: a signed
// value in steps of 8. The erased value is the weakest there is.
constexpr int32_t SOFT_SPLIT = 40;
constexpr int8_t SOFT_WEAK = 20;
constexpr int8_t SOFT_STRONG = 60;
constexpr uint8_t SOFT_ERASED = 0;

inline uint8_t pack_soft(int8_t s) {
    if constexpr (DEINT_SOFT_BITS == 2) {
        const int32_t a = (s < 0) ? -s : s;
        return ((s < 0) ? 2 : 0) | ((a >= SOFT_SPLIT) ? 1 : 0);
    } else {
        int32_t q = (s + ((s < 0) ? -4 : 4)) / 8;
        if (q > 7) q = 7;
        if (q < -7) q = -7;
        return (uint8_t)q & 0x0F;
    }
}

inline int8_t unpack_soft(uint8_t c) {
    if constexpr (DEINT_SOFT_BITS == 2) {
        const int8_t v = (c & 1) ? SOFT_STRONG : SOFT_WEAK;
        return (c & 2) ? -v : v;
    } else {
        return (int8_t)((int8_t)(c << 4) >> 4) * 8;
    }
}

constexpr size_t SOFTS_PER_BYTE = 8 / DEINT_SOFT_BITS;
constexpr uint8_t SOFT_MASK = (1 << DEINT_SOFT_BITS) - 1;

inline void put_soft(uint8_t* segment, size_t depth, uint8_t code) {
    uint8_t& b = segment[depth / SOFTS_PER_BYTE];
    const uint8_t shift = (depth % SOFTS_PER_BYTE) * DEINT_SOFT_BITS;
    b = (b & ~(SOFT_MASK << shift)) | (code << shift);
}

inline uint8_t get_soft(const uint8_t* segment, size_t depth) {
    return (segment[depth / SOFTS_PER_BYTE] >> ((depth % SOFTS_PER_BYTE) * DEINT_SOFT_BITS)) & SOFT_MASK;
}

}  // namespace

// RRC, alpha 0.5, 4 samples a symbol, 8 symbols long, unit DC gain
const float Demodulator::rrc_taps_[RRC_TAPS] = {
    -0.002528f, -0.000956f, 0.002682f, 0.004116f, 0.000758f, -0.004116f, -0.003754f, 0.003870f,
    0.010618f, 0.003870f, -0.018771f, -0.039240f, -0.026546f, 0.039240f, 0.144766f, 0.243806f,
    0.284368f,
    0.243806f, 0.144766f, 0.039240f, -0.026546f, -0.039240f, -0.018771f, 0.003870f, 0.010618f,
    0.003870f, -0.003754f, -0.004116f, 0.000758f, 0.004116f, 0.002682f, -0.000956f, -0.002528f};

// Viterbi

Viterbi::Viterbi(BitHandler handler, void* context)
    : handler_{handler}, context_{context} {
    reset();
}

void Viterbi::reset() {
    memset(metrics_, 0, sizeof(metrics_));
    metric_ = metrics_[0];
    step_ = 0;
    errors_ = 0;
    counted_ = 0;
    bits_ = 0;
    window_ready_ = false;
}

void Viterbi::step(int8_t s0, int8_t s1) {
    const int32_t a = s0;
    const int32_t b = s1;
    const int32_t bm[4] = {-a - b, -a + b, a - b, a + b};

    int32_t* next = (metric_ == metrics_[0]) ? metrics_[1] : metrics_[0];
    uint32_t d[2] = {0, 0};
    for (uint32_t j = 0; j < 32; j++) {
        const int32_t m = bm[code_table.v[j]];
        const int32_t p0 = metric_[j];
        const int32_t p1 = metric_[j + 32];
        const uint32_t ns = j * 2;
        uint32_t* dw = &d[ns >> 5];
        const uint32_t bit = ns & 31;

        int32_t x0 = p0 + m;
        int32_t x1 = p1 - m;
        if (x1 > x0) {
            x0 = x1;
            *dw |= 1u << bit;
        }
        next[ns] = x0;

        int32_t y0 = p0 - m;
        int32_t y1 = p1 + m;
        if (y1 > y0) {
            y0 = y1;
            *dw |= 2u << bit;
        }
        next[ns + 1] = y0;
    }
    metric_ = next;

    const size_t slot = step_ % RING;
    decision_[slot][0] = d[0];
    decision_[slot][1] = d[1];
    received_[slot][0] = s0;
    received_[slot][1] = s1;
    step_++;

    if ((step_ % CHUNK) == 0 && step_ >= RING) traceback();
}

void Viterbi::traceback() {
    uint32_t best = 0;
    for (uint32_t s = 1; s < 64; s++)
        if (metric_[s] > metric_[best]) best = s;
    const int32_t ref = metric_[best];
    for (uint32_t s = 0; s < 64; s++) metric_[s] -= ref;

    uint8_t bits[CHUNK];
    const size_t first = step_ - RING;
    uint32_t s = best;
    for (size_t k = 0; k < RING; k++) {
        const size_t t = step_ - 1 - k;
        const size_t slot = t % RING;
        if (t < first + CHUNK) bits[t - first] = s & 1;
        const uint32_t dbit = (decision_[slot][s >> 5] >> (s & 31)) & 1;
        s = (s >> 1) | (dbit << 5);
    }

    for (size_t i = 0; i < CHUNK; i++) {
        const size_t slot = (first + i) % RING;
        encoder_ = ((encoder_ << 1) | bits[i]) & 0x7F;
        // erased softs (0) say nothing, they are not errors
        const int8_t r0 = received_[slot][0];
        const int8_t r1 = received_[slot][1];
        if (r0) {
            counted_++;
            errors_ += parity(encoder_ & POLY_A) != (r0 > 0);
        }
        if (r1) {
            counted_++;
            errors_ += parity(encoder_ & POLY_B) != (r1 > 0);
        }
        handler_(context_, bits[i]);
    }
    bits_ += CHUNK;
    if (bits_ >= WINDOW_BITS) {
        window_errors_ = errors_;
        window_counted_ = counted_;
        window_ready_ = true;
        errors_ = 0;
        counted_ = 0;
        bits_ = 0;
    }
}

// Deframer

void Deframer::reset() {
    state_ = State::Search;
    shift_ = 0;
    count_ = 0;
    misses_ = 0;
}

void Deframer::bit(uint8_t b) {
    // NRZ-M: a 1 is a change
    const uint8_t x = b ^ last_;
    last_ = b;

    switch (state_) {
        case State::Search:
            shift_ = (shift_ << 1) | x;
            if (__builtin_popcount(shift_ ^ ASM) <= 3) {
                state_ = State::Frame;
                count_ = 0;
                misses_ = 0;
            }
            break;

        case State::Frame: {
            uint8_t& byte = frame_[count_ >> 3];
            byte = (byte << 1) | x;
            if (++count_ == FRAME_BYTES * 8) {
                handler_(context_, frame_);
                state_ = State::Marker;
                count_ = 0;
            }
            break;
        }

        case State::Marker:
            shift_ = (shift_ << 1) | x;
            if (++count_ == 32) {
                count_ = 0;
                if (__builtin_popcount(shift_ ^ ASM) <= 8) {
                    misses_ = 0;
                    state_ = State::Frame;
                } else if (++misses_ > 3) {
                    state_ = State::Search;
                } else {
                    state_ = State::Frame;  // keep going, the RS decides
                }
            }
            break;
    }
}

// Interleaved (80k)

Interleaved::Interleaved(Shared* shared, SoftHandler soft, void* soft_context, BlockHandler block, void* block_context)
    : shared_{shared}, soft_{soft}, soft_context_{soft_context}, block_{block}, block_context_{block_context} {
    for (uint8_t v = 0; v < 4; v++) {
        int8_t m[INTER_MARKER_BITS];
        for (size_t i = 0; i < INTER_MARKER_BITS; i++)
            m[i] = ((INTER_MARKER >> (INTER_MARKER_BITS - 1 - i)) & 1) ? -1 : 1;
        if (v & 2)
            for (size_t i = 0; i < INTER_MARKER_BITS; i += 2) {
                const int8_t t = m[i];
                m[i] = m[i + 1];
                m[i + 1] = t;
            }
        if (v & 1)
            for (size_t i = 1; i < INTER_MARKER_BITS; i += 2) m[i] = -m[i];
        memcpy(pattern_[v], m, sizeof(m));
    }
}

// a new generation: block numbers from 0, all slots free, the M0 starts over
void Interleaved::restart() {
    if (!shared_) return;
    for (size_t i = 0; i < DEINT_SLOTS; i++) {
        shared_->in_state[i] = SLOT_FREE;
        shared_->out_state[i] = SLOT_FREE;
        shared_->in_seq[i] = 0xFFFFFFFF;
        shared_->out_seq[i] = 0xFFFFFFFF;
    }
    shared_->next_consumed = 0;
    __sync_synchronize();
    shared_->generation = shared_->generation + 1;
    seq_ = 0;
    index_ = 0;
    branch_ = 0;
    depth_ = 0;
    stored_ = true;
    using_ = false;
    begun_ = false;
    align_ = false;
}

void Interleaved::soft(int8_t s, bool carrier) {
    carrier_ = carrier;
    switch (state_) {
        case State::Search:
            search(s);
            break;

        case State::Resync:
            // the old grid goes on with erasures, so the blocks keep their timing
            if (phase_ >= INTER_MARKER_BITS && begun_) store(SOFT_ERASED);
            phase_ = (phase_ + 1) % INTER_PERIOD;
            if (++resync_softs_ > RESYNC_SOFTS) {
                state_ = State::Search;
                memset(acc_, 0, sizeof(acc_));
                search_softs_ = 0;
                abs_sum_ = 0;
            }
            if (state_ == State::Resync) search(s);
            break;

        case State::Track:
            check(s);
            if (phase_ < INTER_MARKER_BITS) {
                if (phase_ == 0 && align_) realign();
                marker_corr_ += s * pattern_[variant_][phase_];
                if (phase_ == INTER_MARKER_BITS - 1) {
                    const bool bad = marker_corr_ * sign_ < marker_threshold_;
                    marker_misses_ = (marker_misses_ << 1) | (bad ? 1 : 0);
                    marker_corr_ = 0;
                    if (__builtin_popcount(marker_misses_) >= 6) {
                        state_ = State::Resync;
                        resync_softs_ = 0;
                        memset(acc_, 0, sizeof(acc_));
                        search_softs_ = 0;
                        abs_sum_ = 0;
                    }
                }
            } else {
                data(s);
            }
            phase_ = (phase_ + 1) % INTER_PERIOD;
            break;
    }
    position_ = (position_ + 1) % INTER_PERIOD;
}

// While tracking: the 4 variants a few softs around the markers, over 32 periods. A slip of
// the carrier turns the softs by 90 degrees, and the marker as it then shows still matches the
// old variant in 7 of its 8 bits, one soft over: the misses never see it, this does.
void Interleaved::check(int8_t s) {
    window_[window_index_] = s;
    window_index_ = (window_index_ + 1) % INTER_MARKER_BITS;
    abs_sum_ += (s < 0) ? -s : s;

    const uint8_t start = (position_ + INTER_PERIOD - (INTER_MARKER_BITS - 1)) % INTER_PERIOD;
    int delta = (int)start - (int)start_;
    if (delta > (int)INTER_PERIOD / 2) delta -= INTER_PERIOD;
    if (delta < -(int)INTER_PERIOD / 2) delta += INTER_PERIOD;
    if (delta >= -CHECK_SHIFT && delta <= CHECK_SHIFT) {
        for (uint8_t v = 0; v < 4; v++) {
            int32_t corr = 0;
            for (size_t i = 0; i < INTER_MARKER_BITS; i++)
                corr += window_[(window_index_ + i) % INTER_MARKER_BITS] * pattern_[v][i];
            acc_[v][delta + CHECK_SHIFT] += (int16_t)corr;
        }
    }

    if (++search_softs_ < SEARCH_PERIODS * INTER_PERIOD) return;

    int32_t best = 0;
    uint8_t best_v = 0;
    int best_delta = 0;
    for (uint8_t v = 0; v < 4; v++)
        for (int d = -CHECK_SHIFT; d <= CHECK_SHIFT; d++) {
            const int32_t a = (acc_[v][d + CHECK_SHIFT] < 0) ? -acc_[v][d + CHECK_SHIFT] : acc_[v][d + CHECK_SHIFT];
            if (a > best) {
                best = a;
                best_v = v;
                best_delta = d;
            }
        }
    const int32_t current = (acc_[variant_][CHECK_SHIFT] < 0) ? -acc_[variant_][CHECK_SHIFT] : acc_[variant_][CHECK_SHIFT];
    const int8_t sign = (acc_[best_v][best_delta + CHECK_SHIFT] < 0) ? -1 : 1;
    const int32_t expected = SEARCH_PERIODS * INTER_MARKER_BITS * (int32_t)(abs_sum_ / search_softs_);

    memset(acc_, 0, sizeof(acc_));
    search_softs_ = 0;
    abs_sum_ = 0;
    if (carrier_ && (best_v != variant_ || best_delta) && (best - current) * 16 > expected) {
        start_ = (start_ + INTER_PERIOD + best_delta) % INTER_PERIOD;
        variant_ = best_v;
        sign_ = sign;
        phase_ = (phase_ + INTER_PERIOD - best_delta) % INTER_PERIOD;
        marker_corr_ = 0;
        align_ = true;
    } else if (best_v == variant_ && !best_delta) {
        sign_ = sign;
    }
}

// Correlates each position of the period with the 4 variants of the marker, over 32 periods.
void Interleaved::search(int8_t s) {
    window_[window_index_] = s;
    window_index_ = (window_index_ + 1) % INTER_MARKER_BITS;
    abs_sum_ += (s < 0) ? -s : s;

    const uint8_t start = (position_ + INTER_PERIOD - (INTER_MARKER_BITS - 1)) % INTER_PERIOD;
    for (uint8_t v = 0; v < 4; v++) {
        int32_t corr = 0;
        for (size_t i = 0; i < INTER_MARKER_BITS; i++)
            corr += window_[(window_index_ + i) % INTER_MARKER_BITS] * pattern_[v][i];
        acc_[v][start] += (int16_t)corr;
    }

    if (++search_softs_ < SEARCH_PERIODS * INTER_PERIOD) return;

    int32_t best = 0;
    int32_t second = 0;
    uint8_t best_v = 0;
    uint8_t best_start = 0;
    for (uint8_t v = 0; v < 4; v++)
        for (uint8_t p = 0; p < INTER_PERIOD; p++) {
            const int32_t a = (acc_[v][p] < 0) ? -acc_[v][p] : acc_[v][p];
            if (a > best) {
                second = best;
                best = a;
                best_v = v;
                best_start = p;
            } else if (a > second) {
                second = a;
            }
        }
    const int8_t sign = (acc_[best_v][best_start] < 0) ? -1 : 1;
    const int32_t mean_abs = abs_sum_ / search_softs_;
    const int32_t expected = SEARCH_PERIODS * INTER_MARKER_BITS * mean_abs;

    memset(acc_, 0, sizeof(acc_));
    search_softs_ = 0;
    abs_sum_ = 0;
    // The marker turned by 90 degrees and moved by one soft still matches 7 of its 8 bits, so
    // the runner up is close; one bit apart is expected / 8, half of that will do.
    if (carrier_ && best * 2 > expected && (best - second) * 16 > expected)
        found(best_start, best_v, sign, INTER_MARKER_BITS * mean_abs / 4);
}

void Interleaved::found(uint8_t start, uint8_t variant, int8_t sign, int32_t threshold) {
    if (state_ == State::Resync) {
        int delta = (int)start - (int)start_;
        if (delta > (int)INTER_PERIOD / 2) delta -= INTER_PERIOD;
        if (delta < -(int)INTER_PERIOD / 2) delta += INTER_PERIOD;
        if (delta > RESYNC_SHIFT || delta < -RESYNC_SHIFT)
            restart();
        else
            align_ = true;
    } else {
        restart();
    }
    state_ = State::Track;
    start_ = start;
    variant_ = variant;
    sign_ = sign;
    marker_threshold_ = threshold;
    marker_misses_ = 0;
    marker_corr_ = 0;
    memset(acc_, 0, sizeof(acc_));
    search_softs_ = 0;
    abs_sum_ = 0;
    // the soft that was just searched sat at position_
    phase_ = (position_ + 1 + INTER_PERIOD - start) % INTER_PERIOD;
}

// After a slip the markers moved by a soft or two: back onto a period boundary of the blocks,
// with erasures, or by writing over the last few softs.
void Interleaved::realign() {
    align_ = false;
    const size_t rem = index_ % INTER_DATA;
    if (!rem) return;
    if (rem <= INTER_DATA / 2 && index_ >= rem) {
        index_ -= rem;
        branch_ = index_ % INTER_BRANCHES;
        depth_ = index_ / INTER_BRANCHES;
    } else {
        for (size_t k = rem; k < INTER_DATA; k++) store(SOFT_ERASED);
    }
}

void Interleaved::data(int8_t s) {
    if (!begun_) {
        // wait for a marker, the blocks start on one
        if (phase_ != INTER_MARKER_BITS) return;
        begun_ = true;
    }
    const uint8_t e = phase_ - INTER_MARKER_BITS;
    if (!(e & 1)) {
        held_ = s;
        return;
    }
    int8_t a = held_;
    int8_t b = s;
    if (variant_ & 1) b = -b;
    if (variant_ & 2) {
        const int8_t t = a;
        a = b;
        b = t;
    }
    // The polarity too: the deinterleaver mixes softs from before and after a slip, an inversion
    // is only harmless to the decoder when it is the same for all of them.
    if (sign_ < 0) {
        a = -a;
        b = -b;
    }
    store(pack_soft(a));
    store(pack_soft(b));
}

void Interleaved::store(uint8_t code) {
    if (stored_) put_soft(shared_->in[seq_ % DEINT_SLOTS] + branch_ * DEINT_SEGMENT, depth_, code);
    use();
    index_++;
    if (++branch_ == INTER_BRANCHES) {
        branch_ = 0;
        depth_++;
    }
    if (index_ == DEINT_BLOCK_SOFTS) complete();
}

// one deinterleaved soft into the decoder, which pairs them
void Interleaved::use() {
    if (!using_ || use_index_ == DEINT_BLOCK_SOFTS) return;
    const size_t slot = use_seq_ % DEINT_SLOTS;
    int8_t v = 0;
    if (use_branch_ == INTER_BRANCHES - 1) {
        if (use_stored_) v = unpack_soft(get_soft(shared_->in[slot] + use_branch_ * DEINT_SEGMENT, use_depth_));
    } else if (use_ready_ && ((use_valid_ >> use_branch_) & 1)) {
        v = unpack_soft(get_soft(shared_->out[slot] + use_branch_ * DEINT_SEGMENT, use_depth_));
    }
    soft_(soft_context_, v);
    use_index_++;
    if (++use_branch_ == INTER_BRANCHES) {
        use_branch_ = 0;
        use_depth_++;
    }
}

void Interleaved::complete() {
    const size_t slot = seq_ % DEINT_SLOTS;

    // done with the output block used meanwhile
    if (using_) {
        const size_t used = use_seq_ % DEINT_SLOTS;
        if (shared_->out_seq[used] == use_seq_) shared_->out_state[used] = SLOT_FREE;
    }

    if (stored_) {
        shared_->in_seq[slot] = seq_;
        __sync_synchronize();
        shared_->in_state[slot] = SLOT_FULL;
    }

    // An output block the M0 finished too late, or from before a restart, would hold its slot
    // for good: only this block and the next may be ready.
    for (size_t i = 0; i < DEINT_SLOTS; i++) {
        const uint32_t q = shared_->out_seq[i];
        if (shared_->out_state[i] == SLOT_READY && q != seq_ && q != seq_ + 1) shared_->out_state[i] = SLOT_FREE;
    }

    // its output block is used while the next input block fills
    using_ = true;
    use_seq_ = seq_;
    use_stored_ = stored_;
    use_index_ = 0;
    use_branch_ = 0;
    use_depth_ = 0;
    use_ready_ = shared_->out_state[slot] == SLOT_READY && shared_->out_seq[slot] == seq_;
    __sync_synchronize();
    use_valid_ = shared_->out_valid[slot];
    if (!use_ready_ && seq_) late_++;  // block 0 can only be built once it is in
    shared_->next_consumed = seq_ + 1;
    block_(block_context_);

    seq_++;
    index_ = 0;
    branch_ = 0;
    depth_ = 0;
    const size_t next = seq_ % DEINT_SLOTS;
    stored_ = shared_->in_state[next] != SLOT_FULL;
    if (stored_)
        shared_->in_state[next] = SLOT_FREE;
    else
        lost_++;
}

// Demodulator

Demodulator::Demodulator(Mode mode, FrameHandler handler, void* context, Shared* shared, BlockHandler block)
    : mode_{mode},
      fll_near_{6.2831853f * 500.0f / input_rate(mode)},
      deframer_{handler, context},
      viterbi_{on_bit, this},
      interleaved_{shared, on_soft, this, block, context} {
}

void Demodulator::on_bit(void* context, uint8_t bit) {
    static_cast<Demodulator*>(context)->deframer_.bit(bit);
}

void Demodulator::on_soft(void* context, int8_t s) {
    static_cast<Demodulator*>(context)->soft(s);
}

// 72k: the code locked. 80k: the code only locks once the deinterleaver is full, the markers
// tell much earlier that the carrier is right.
bool Demodulator::tracking() const {
    return (mode_ == Mode::M80K) ? interleaved_.locked() : code_locked_;
}

int32_t Demodulator::frequency_offset_hz() const {
    return (int32_t)(omega_ * (float)input_rate(mode_) / 6.2831853f);
}

void Demodulator::constellation(int8_t* out) const {
    memcpy(out, points_, sizeof(points_));
}

void Demodulator::set_omega(float omega) {
    if (omega > OMEGA_MAX) omega = OMEGA_MAX;
    if (omega < -OMEGA_MAX) omega = -OMEGA_MAX;
    omega_ = omega;
    const float w2 = omega * omega;
    // e^-jw, the series is plenty for |w| < 0.2
    nco_step_.re = 1.0f - w2 * (0.5f - w2 * (1.0f / 24.0f - w2 * (1.0f / 720.0f)));
    nco_step_.im = -omega * (1.0f - w2 * (1.0f / 6.0f - w2 * (1.0f / 120.0f)));
}

void Demodulator::execute(const Cf& in) {
    // carrier NCO ahead of the matched filter, so the filter sits on the signal
    const Cf x = mul(in, nco_);
    nco_ = mul(nco_, nco_step_);

    fll_acc_.re += x.re * fll_last_.re + x.im * fll_last_.im;
    fll_acc_.im += x.im * fll_last_.re - x.re * fll_last_.im;
    fll_last_ = x;
    if (++fll_count_ == FLL_SAMPLES) {
        if (!tracking()) {
            const float e = atan2f(fll_acc_.im, fll_acc_.re);
            set_omega(omega_ + e * ((fabsf(e) > fll_near_) ? FLL_GAIN_FAR : FLL_GAIN_NEAR));
        }
        fll_acc_ = {0.0f, 0.0f};
        fll_count_ = 0;
    }

    rrc_index_ = (rrc_index_ == 0) ? RRC_TAPS - 1 : rrc_index_ - 1;
    rrc_history_[rrc_index_] = x;
    rrc_history_[rrc_index_ + RRC_TAPS] = x;
    rrc_phase_ = !rrc_phase_;
    if (!rrc_phase_) return;

    // symmetric taps: pairs share a multiply
    const Cf* h = &rrc_history_[rrc_index_];
    constexpr size_t half = RRC_TAPS / 2;
    Cf acc{rrc_taps_[half] * h[half].re, rrc_taps_[half] * h[half].im};
    for (size_t k = 0; k < half; k++) {
        acc.re += rrc_taps_[k] * (h[k].re + h[RRC_TAPS - 1 - k].re);
        acc.im += rrc_taps_[k] * (h[k].im + h[RRC_TAPS - 1 - k].im);
    }
    sample(acc);
}

void Demodulator::sample(Cf y) {
    power_ += 0.002f * (y.re * y.re + y.im * y.im - power_);
    if (++gain_count_ == 16) {
        gain_count_ = 0;
        gain_ = 1.0f / sqrtf(power_ + 1e-20f);
    }
    y.re *= gain_;
    y.im *= gain_;

    // OQPSK: I one sample (half a symbol) late, so both rails peak together
    const float i = delayed_i_;
    delayed_i_ = y.re;
    y.re = i;

    history_[0] = history_[1];
    history_[1] = history_[2];
    history_[2] = history_[3];
    history_[3] = y;

    next_ -= 1.0f;
    while (next_ < 1.0f) {
        const float mu = (next_ < 0.0f) ? 0.0f : next_;
        // cubic Lagrange between history_[1] and history_[2]
        Cf v;
        const Cf& p0 = history_[0];
        const Cf& p1 = history_[1];
        const Cf& p2 = history_[2];
        const Cf& p3 = history_[3];
        {
            const float c1 = p2.re - p0.re * (1.0f / 3.0f) - p1.re * 0.5f - p3.re * (1.0f / 6.0f);
            const float c2 = (p0.re + p2.re) * 0.5f - p1.re;
            const float c3 = (p3.re - p0.re) * (1.0f / 6.0f) + (p1.re - p2.re) * 0.5f;
            v.re = ((c3 * mu + c2) * mu + c1) * mu + p1.re;
        }
        {
            const float c1 = p2.im - p0.im * (1.0f / 3.0f) - p1.im * 0.5f - p3.im * (1.0f / 6.0f);
            const float c2 = (p0.im + p2.im) * 0.5f - p1.im;
            const float c3 = (p3.im - p0.im) * (1.0f / 6.0f) + (p1.im - p2.im) * 0.5f;
            v.im = ((c3 * mu + c2) * mu + c1) * mu + p1.im;
        }
        next_ += 1.0f + period_adjust_;
        interpolant(v);
    }
}

void Demodulator::interpolant(const Cf& y) {
    if (!strobe_next_) {
        mid_ = y;
        strobe_next_ = true;
        return;
    }
    strobe_next_ = false;

    // Gardner: positive when the strobes are late
    const float e = (y.re - last_symbol_.re) * mid_.re + (y.im - last_symbol_.im) * mid_.im;
    last_symbol_ = y;
    period_adjust_ -= TIMING_KI * e;
    if (period_adjust_ > TIMING_MAX_ADJUST) period_adjust_ = TIMING_MAX_ADJUST;
    if (period_adjust_ < -TIMING_MAX_ADJUST) period_adjust_ = -TIMING_MAX_ADJUST;
    next_ -= TIMING_KP * e;

    symbol(y);
}

void Demodulator::symbol(const Cf& y) {
    // carrier: decision directed QPSK, points on the diagonals
    const float ec = ((y.re > 0.0f) ? y.im : -y.im) - ((y.im > 0.0f) ? y.re : -y.re);
    const bool narrow = tracking();
    const Cf turn{1.0f, -(narrow ? CARRIER_ALPHA : CARRIER_ALPHA_ACQUIRE) * ec};
    nco_ = mul(nco_, turn);
    const float mag = nco_.re * nco_.re + nco_.im * nco_.im;
    const float fix = 1.5f - 0.5f * mag;
    nco_.re *= fix;
    nco_.im *= fix;
    set_omega(omega_ + (narrow ? CARRIER_BETA : CARRIER_BETA_ACQUIRE) * ec);

    // lock: the points sit on the diagonals, |I| close to |Q|
    const float ai = fabsf(y.re);
    const float aq = fabsf(y.im);
    lock_num_ += (2.0f * ai * aq - lock_num_) * (1.0f / 512.0f);
    lock_den_ += (y.re * y.re + y.im * y.im - lock_den_) * (1.0f / 512.0f);
    const float ratio = lock_num_ / (lock_den_ + 1e-20f);
    carrier_locked_ = carrier_locked_ ? (ratio > 0.67f) : (ratio > 0.70f);

    const int8_t si = clamp_soft(y.re * SOFT_SCALE);
    const int8_t sq = clamp_soft(y.im * SOFT_SCALE);
    points_[point_index_][0] = si;
    points_[point_index_][1] = sq;
    point_index_ = (point_index_ + 1) % CONSTELLATION_POINTS;

    soft_in(si, carrier_locked_);
    soft_in(sq, carrier_locked_);
}

void Demodulator::soft_in(int8_t s, bool carrier) {
    if (mode_ == Mode::M80K)
        interleaved_.soft(s, carrier);
    else
        soft(s);
}

void Demodulator::soft(int8_t s) {
    if (skip_) {
        skip_ = false;
        return;
    }
    if (!have_first_) {
        first_ = s;
        have_first_ = true;
        return;
    }
    have_first_ = false;

    int8_t a = first_;
    int8_t b = s;
    if (hypothesis_ & 2) {
        const int8_t t = a;
        a = b;
        b = t;
    }
    if (hypothesis_ & 1) b = -b;
    decode(a, b);
}

void Demodulator::decode(int8_t a, int8_t b) {
    viterbi_.step(a, b);
    if (!viterbi_.window_done()) return;

    // mostly erasures (80k, blocks late or not in yet): no news about the lock
    const uint32_t counted = viterbi_.window_counted();
    if (counted < Viterbi::WINDOW_BITS) return;
    ber_permille_ = (uint16_t)((viterbi_.window_errors() * 1000) / counted);
    if (ber_permille_ < BER_LOCK_PERMILLE) {
        code_locked_ = true;
        bad_windows_ = 0;
    } else if (!code_locked_ || ++bad_windows_ > 4) {
        code_locked_ = false;
        bad_windows_ = 0;
        next_hypothesis();
    }
}

void Demodulator::next_hypothesis() {
    hypothesis_ = (hypothesis_ + 1) & 7;
    const bool shift = (hypothesis_ >> 2) & 1;
    if (shift != shift_) {
        shift_ = shift;
        skip_ = true;
    }
    viterbi_.reset();
    deframer_.reset();
}

}  // namespace lrpt
