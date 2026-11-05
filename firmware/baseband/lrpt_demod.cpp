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
constexpr float OMEGA_MAX = 0.2f;  // rad/sample at 288 kS/s, about 9 kHz

// FLL, until the code locks: the phase of the lag 1 autocorrelation ahead of the matched
// filter, over 32768 samples. The spectrum is symmetric, so it needs neither the timing nor the
// carrier phase. Big steps far out, small ones close in, where the estimates are mostly noise
// and the carrier loop pulls in the rest.
constexpr uint16_t FLL_SAMPLES = 32768;
constexpr float FLL_NEAR = 6.2831853f * 500.0f / INPUT_RATE;
constexpr float FLL_GAIN_FAR = 0.6f;
constexpr float FLL_GAIN_NEAR = 0.15f;

constexpr float SOFT_SCALE = 70.0f;
constexpr uint16_t BER_LOCK_PERMILLE = 120;

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
        errors_ += (parity(encoder_ & POLY_A) != (received_[slot][0] > 0)) +
                   (parity(encoder_ & POLY_B) != (received_[slot][1] > 0));
        handler_(context_, bits[i]);
    }
    bits_ += CHUNK;
    if (bits_ >= WINDOW_BITS) {
        window_errors_ = errors_;
        window_ready_ = true;
        errors_ = 0;
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

// Demodulator

Demodulator::Demodulator(FrameHandler handler, void* context)
    : deframer_{handler, context},
      viterbi_{on_bit, this} {
}

void Demodulator::on_bit(void* context, uint8_t bit) {
    static_cast<Demodulator*>(context)->deframer_.bit(bit);
}

int32_t Demodulator::frequency_offset_hz() const {
    return (int32_t)(omega_ * (float)INPUT_RATE / 6.2831853f);
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
        if (!code_locked_) {
            const float e = atan2f(fll_acc_.im, fll_acc_.re);
            set_omega(omega_ + e * ((fabsf(e) > FLL_NEAR) ? FLL_GAIN_FAR : FLL_GAIN_NEAR));
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
    const Cf turn{1.0f, -(code_locked_ ? CARRIER_ALPHA : CARRIER_ALPHA_ACQUIRE) * ec};
    nco_ = mul(nco_, turn);
    const float mag = nco_.re * nco_.re + nco_.im * nco_.im;
    const float fix = 1.5f - 0.5f * mag;
    nco_.re *= fix;
    nco_.im *= fix;
    set_omega(omega_ + (code_locked_ ? CARRIER_BETA : CARRIER_BETA_ACQUIRE) * ec);

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

    soft(si);
    soft(sq);
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
    viterbi_.step(a, b);

    if (viterbi_.window_done()) {
        ber_permille_ = (uint16_t)((viterbi_.window_errors() * 1000) / (2 * Viterbi::WINDOW_BITS));
        if (ber_permille_ < BER_LOCK_PERMILLE) {
            code_locked_ = true;
            bad_windows_ = 0;
        } else if (!code_locked_ || ++bad_windows_ > 4) {
            code_locked_ = false;
            bad_windows_ = 0;
            next_hypothesis();
        }
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
