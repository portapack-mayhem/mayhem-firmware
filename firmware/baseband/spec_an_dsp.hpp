/*
 * Copyright (C) 2026 zxkmm
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

/* Spectrum analyzer DSP: FFT, windowing, power -> display points.
 * Header only and free of ChibiOS so the host tests run the same code. */

#ifndef __SPEC_AN_DSP_H__
#define __SPEC_AN_DSP_H__

#include "spec_an_shared.hpp"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace spec_an {
namespace dsp {

struct CFloat {
    float re;
    float im;
};

inline uint32_t bit_reverse(uint32_t v, size_t bits) {
#if defined(__arm__)
    uint32_t r;
    __asm__("rbit %0, %1" : "=r"(r) : "r"(v));
    return r >> (32 - bits);
#else
    uint32_t r = 0;
    for (size_t i = 0; i < bits; i++) {
        r = (r << 1) | (v & 1);
        v >>= 1;
    }
    return r;
#endif
}

/* Natural log to ~6e-5 (0.0003 dB once scaled): exponent plus a quartic in
 * the mantissa. */
inline float ln_approx(float x) {
    uint32_t i;
    std::memcpy(&i, &x, sizeof(i));
    const float e = static_cast<float>(static_cast<int32_t>((i >> 23) & 0xff) - 127);
    i = (i & 0x007fffffu) | 0x3f800000u;
    float m;
    std::memcpy(&m, &i, sizeof(m));
    const float ln_m = -1.7417939f + (2.8212026f + (-1.4699568f + (0.44717955f - 0.056570851f * m) * m) * m) * m;
    return e * 0.69314718f + ln_m;
}

/* sin(2 pi i / kMaxFft) for i in [0, kMaxFft / 4]. Taylor to x^11 on
 * [0, pi/2] is within float rounding, and keeps libm's sin (with its
 * argument reduction, ~5 KiB) out of the image. */
inline void init_sin_table(float* sin_q) {
    constexpr size_t q = kMaxFft / 4;
    constexpr float step = 6.28318530717958647f / kMaxFft;
    for (size_t i = 0; i <= q; i++) {
        const float x = step * static_cast<float>(i);
        const float x2 = x * x;
        sin_q[i] = x * (1.0f + x2 * (-1.0f / 6 + x2 * (1.0f / 120 + x2 * (-1.0f / 5040 + x2 * (1.0f / 362880 + x2 * (-1.0f / 39916800))))));
    }
}

/* Interleaved int8 I/Q in, float in bit-reversed order out. */
inline void load_block(CFloat* x, const int8_t* iq, size_t log2n) {
    const size_t n = size_t{1} << log2n;
    for (size_t i = 0; i < n; i++) {
        const uint32_t r = bit_reverse(i, log2n);
        x[r].re = iq[2 * i];
        x[r].im = iq[2 * i + 1];
    }
}

/* In-place radix-2 DIT on bit-reversed input. The first two stages only
 * need twiddles 1 and -j, so they run as a multiply-free radix-4 pass. */
inline void fft(CFloat* x, size_t log2n, const float* sin_q) {
    const size_t n = size_t{1} << log2n;

    for (size_t i = 0; i < n; i += 4) {
        const CFloat a = x[i + 0];
        const CFloat b = x[i + 1];
        const CFloat c = x[i + 2];
        const CFloat d = x[i + 3];
        const float t0r = a.re + b.re, t0i = a.im + b.im;
        const float t1r = a.re - b.re, t1i = a.im - b.im;
        const float t2r = c.re + d.re, t2i = c.im + d.im;
        const float t3r = c.re - d.re, t3i = c.im - d.im;
        x[i + 0] = {t0r + t2r, t0i + t2i};
        x[i + 2] = {t0r - t2r, t0i - t2i};
        x[i + 1] = {t1r + t3i, t1i - t3r};
        x[i + 3] = {t1r - t3i, t1i + t3r};
    }

    constexpr size_t q = kMaxFft / 4;
    for (size_t m = 4; m < n; m <<= 1) {
        const size_t stride = kMaxFft / (2 * m);
        const size_t step = 2 * m;
        for (size_t k = 0; k < m; k++) {
            /* w = exp(-j 2 pi a / kMaxFft), from the quarter wave table. */
            const size_t a = k * stride;
            float c, s;
            if (a <= q) {
                c = sin_q[q - a];
                s = sin_q[a];
            } else {
                c = -sin_q[a - q];
                s = sin_q[2 * q - a];
            }
            for (size_t i = k; i < n; i += step) {
                CFloat& u = x[i];
                CFloat& v = x[i + m];
                const float tr = c * v.re + s * v.im;
                const float ti = c * v.im - s * v.re;
                v.re = u.re - tr;
                v.im = u.im - ti;
                u.re += tr;
                u.im += ti;
            }
        }
    }
}

inline float coherent_gain(Window window) {
    switch (window) {
        case Window::Hann:
            return 0.5f;
        case Window::BlackmanHarris:
            return 0.35875f;
        case Window::FlatTop:
            return 0.21557895f;
        default:
            return 1.0f;
    }
}

/* Windowing as a convolution in the frequency domain, only over the bins
 * reported. a_k cosine terms become -/+ a_k/2 neighbour taps. Bin indices
 * wrap modulo N so a run may start below the LO. */
template <size_t T>
inline void window_power_taps(const CFloat* x, size_t n, int32_t first, size_t count, const float (&c)[T + 1], float* acc) {
    const size_t mask = n - 1;
    for (size_t i = 0; i < count; i++) {
        const size_t k = static_cast<size_t>(first + static_cast<int32_t>(i)) & mask;
        float re = c[0] * x[k].re;
        float im = c[0] * x[k].im;
        for (size_t t = 1; t <= T; t++) {
            const CFloat& lo = x[(k - t) & mask];
            const CFloat& hi = x[(k + t) & mask];
            re += c[t] * (lo.re + hi.re);
            im += c[t] * (lo.im + hi.im);
        }
        acc[i] += re * re + im * im;
    }
}

inline void window_power(const CFloat* x, size_t n, int32_t first, size_t count, Window window, float* acc) {
    static constexpr float rect[1] = {1.0f};
    static constexpr float hann[2] = {0.5f, -0.25f};
    static constexpr float bh4[4] = {0.35875f, -0.244145f, 0.07064f, -0.00584f};
    static constexpr float flattop[5] = {0.21557895f, -0.20831579f, 0.138631579f, -0.0417894735f, 0.003473684f};

    switch (window) {
        case Window::Hann:
            window_power_taps<1>(x, n, first, count, hann, acc);
            break;
        case Window::BlackmanHarris:
            window_power_taps<3>(x, n, first, count, bh4, acc);
            break;
        case Window::FlatTop:
            window_power_taps<4>(x, n, first, count, flattop, acc);
            break;
        default:
            window_power_taps<0>(x, n, first, count, rect, acc);
            break;
    }
}

/* dB offset that makes a full scale complex tone (|x| = 127) centred in a
 * bin read 0 after `avg` power sums. */
inline float norm_db(size_t n, Window window, size_t avg) {
    /* -20 log10(a) - 10 log10(avg), via ln_approx to keep libm out. */
    return -8.68588964f * ln_approx(127.0f * static_cast<float>(n) * coherent_gain(window)) - 4.34294482f * ln_approx(static_cast<float>(avg));
}

/* Streams power bins, in ascending global bin order, slice by slice, into
 * display points (centi-dB). Global bin g sits at display coordinate
 * g * px_per_bin, where point x owns [x, x + 1).
 *
 *  dense  (px_per_bin <= 1): bins combined per point in linear power by the
 *         detector, one log per point;
 *  sparse (px_per_bin > 1):  points interpolated in dB between bin centres.
 *
 * State is snapshotted at each slice start, so running the same slice of
 * the same sweep again (a re-request) rewinds instead of double counting. */
class PointDetector {
   public:
    void configure(float px_per_bin, Detector detector, float norm_db, size_t slices) {
        px_per_bin_ = px_per_bin;
        bins_per_px_ = 1.0f / px_per_bin;
        sparse_ = px_per_bin > 1.0f;
        detector_ = detector;
        norm_db_ = norm_db;
        slices_ = std::max<size_t>(slices, 1);
        st_ = State{};
        snap_slice_ = -1;
    }

    /* out: kPoints display points for this sweep. */
    void run(const float* power, size_t count, uint16_t slice, uint8_t sweep, int16_t* out) {
        if (static_cast<int32_t>(slice) == snap_slice_ && sweep == snap_sweep_) {
            st_ = snap_;
        } else {
            if (slice == 0) st_ = State{};
            snap_ = st_;
            snap_slice_ = slice;
            snap_sweep_ = sweep;
        }

        out_ = out;
        px_begin_ = 0;
        px_end_ = 0;
        const bool last = (slice + 1u) >= slices_;
        const uint32_t g0 = static_cast<uint32_t>(slice) * count;
        if (sparse_)
            run_sparse(power, count, g0, last);
        else
            run_dense(power, count, g0, last);
    }

    /* Points [px_begin, px_end) were final after the last run(). */
    uint16_t px_begin() const { return px_begin_; }
    uint16_t px_end() const { return px_end_; }

    float to_db(float power) const {
        return 4.34294482f * ln_approx(std::max(power, 1e-30f)) + norm_db_; /* 10 / ln(10) */
    }

   private:
    struct State {
        int32_t cur_px{-1}; /* dense: point being accumulated */
        float acc{0.0f};
        uint32_t count{0};
        int32_t next_px{0}; /* sparse: next point to interpolate */
        float prev_db{0.0f};
        float last_db{-200.0f};
    };

    float px_per_bin_{1.0f};
    float bins_per_px_{1.0f};
    bool sparse_{false};
    Detector detector_{Detector::Peak};
    float norm_db_{0.0f};
    size_t slices_{1};

    State st_{};
    State snap_{};
    int32_t snap_slice_{-1};
    uint8_t snap_sweep_{0};

    int16_t* out_{nullptr};
    uint16_t px_begin_{0};
    uint16_t px_end_{0};

    static constexpr int32_t points = static_cast<int32_t>(kPoints);

    void emit(int32_t px, float db) {
        if (px < 0 || px >= points) return;
        out_[px] = static_cast<int16_t>(std::clamp(db * 100.0f, -32000.0f, 32000.0f));
        st_.last_db = db;
        if (px_end_ == px_begin_) px_begin_ = px;
        px_end_ = px + 1;
    }

    void emit_acc() {
        float p = st_.acc;
        if (detector_ == Detector::Average && st_.count) p /= static_cast<float>(st_.count);
        emit(st_.cur_px, to_db(p));
    }

    void run_dense(const float* power, size_t count, uint32_t g0, bool last) {
        for (size_t i = 0; i < count; i++) {
            const int32_t px = static_cast<int32_t>(static_cast<float>(g0 + i) * px_per_bin_);
            const float p = power[i];
            if (px != st_.cur_px) {
                if (st_.cur_px >= 0) emit_acc();
                st_.cur_px = px;
                if (px >= points) break;
                st_.acc = p;
                st_.count = 1;
                continue;
            }
            switch (detector_) {
                case Detector::Peak:
                    st_.acc = std::max(st_.acc, p);
                    break;
                case Detector::NegPeak:
                    st_.acc = std::min(st_.acc, p);
                    break;
                case Detector::Average:
                    st_.acc += p;
                    break;
                default: /* Sample keeps the first bin */
                    break;
            }
            st_.count++;
        }

        if (last) {
            if (st_.cur_px >= 0 && st_.cur_px < points) emit_acc();
            /* Only if the plan came up short: never leave stale points. */
            for (int32_t px = std::max<int32_t>(st_.cur_px + 1, 0); px < points; px++) emit(px, st_.last_db);
            st_.cur_px = points;
        }
    }

    void run_sparse(const float* power, size_t count, uint32_t g0, bool last) {
        for (size_t i = 0; i < count; i++) {
            const uint32_t g = g0 + i;
            const float db = to_db(power[i]);
            while (st_.next_px < points) {
                const float u = (static_cast<float>(st_.next_px) + 0.5f) * bins_per_px_;
                if (u > static_cast<float>(g)) break;
                const float frac = u - static_cast<float>(g) + 1.0f;
                emit(st_.next_px, st_.prev_db + (db - st_.prev_db) * frac);
                st_.next_px++;
            }
            st_.prev_db = db;
        }

        if (last) {
            while (st_.next_px < points) {
                emit(st_.next_px, st_.prev_db);
                st_.next_px++;
            }
        }
    }
};

} /* namespace dsp */
} /* namespace spec_an */

#endif /*__SPEC_AN_DSP_H__*/
