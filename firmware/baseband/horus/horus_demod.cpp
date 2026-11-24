/*
 * Copyright (C) 2026 OE1LBS
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

#include "horus_demod.hpp"

#include <cmath>
#include <cstring>

namespace horus {

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kBinHz = (float)kFs / (float)kFftN;
constexpr float kSpecAlpha = 0.03f;   // spectrum averaging per FFT block (~1.4 s time constant)
constexpr float kDetectRatio = 4.0f;  // comb tone power over noise needed to (re)estimate tones
constexpr float kTimingGain = 0.3f;   // symbol clock loop gain
constexpr float kSnrAlpha = 0.02f;    // SNR averaging per symbol

inline cf32 cmul(cf32 a, cf32 b) {
    return {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
}

inline float mag2(cf32 a) {
    return a.re * a.re + a.im * a.im;
}

constexpr int kFftLog2 = 9;
static_assert((1 << kFftLog2) == kFftN, "kFftN must be 2^kFftLog2");

// Job steps: 0 bit reversal, 1..kFftLog2 butterfly stages, then spectrum
// accumulation, comb search and tone refinement.
constexpr int kStepAccumulate = kFftLog2 + 1;
constexpr int kStepComb = kFftLog2 + 2;
constexpr int kStepRefine = kFftLog2 + 3;
static_assert(kStepRefine + 1 == Demod4FSK::kSpectrumSteps, "step count");

// Spectrum value at a fractional bin (shifted order, bin kFftN/2 = 0 Hz), linear interpolation.
inline float spec_at(const float* s, float b) {
    int i = (int)b;
    if (i < 0) return 0.0f;
    if (i >= kFftN - 1) return s[kFftN - 1];
    const float f = b - (float)i;
    return s[i] * (1.0f - f) + s[i + 1] * f;
}

}  // namespace

Demod4FSK::Demod4FSK() {
    // Tables once, so no trig runs on the real-time path.
    for (int i = 0; i < kFftN; i++)
        window_[i] = 0.5f - 0.5f * cosf(2.0f * kPi * (float)i / (float)(kFftN - 1));
    for (int i = 0; i < kFftN / 2; i++) {
        const float a = -2.0f * kPi * (float)i / (float)kFftN;
        twiddle_[i] = {cosf(a), sinf(a)};
    }
    reset();
}

void Demod4FSK::reset() {
    memset(spec_, 0, sizeof(spec_));
    memset(ring_, 0, sizeof(ring_));
    memset(acc_, 0, sizeof(acc_));
    memset(metric_, 0, sizeof(metric_));
    memset(sym_, 0, sizeof(sym_));
    memset(e_best_, 0, sizeof(e_best_));
    memset(e_rest_, 0, sizeof(e_rest_));
    fft_fill_ = 0;
    job_step_ = -1;
    comb_best_ = 0;
    ring_pos_ = 0;
    hist_pos_ = 0;
    samples_to_eval_ = kTs;
    tpos_ = 0.5f * kTs;
    snr_sig_ = 0.0f;
    snr_noise_ = 1e-12f;
    status_ = {};
    // Until a signal is found, park the tones at a nominal comb just above the centre.
    for (int k = 0; k < kTones; k++) {
        osc_[k] = {1.0f, 0.0f};
        set_tone(k, 250.0f + k * tone_spacing_hz_);
    }
}

void Demod4FSK::set_tone(int k, float hz) {
    status_.tone_hz[k] = hz;
    const float w = -2.0f * kPi * hz / (float)kFs;
    step_[k] = {cosf(w), sinf(w)};
}

void Demod4FSK::background_step() {
    if (job_step_ < 0) return;
    cf32* x = fft_work_;

    if (job_step_ == 0) {
        // bit-reversal permutation
        for (int i = 1, j = 0; i < kFftN; i++) {
            int bit = kFftN >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) {
                const cf32 t = x[i];
                x[i] = x[j];
                x[j] = t;
            }
        }
    } else if (job_step_ <= kFftLog2) {
        // one radix-2 butterfly stage
        const int len = 1 << job_step_;
        const int half = len / 2;
        const int tw_stride = kFftN / len;
        for (int i = 0; i < kFftN; i += len) {
            for (int j = 0; j < half; j++) {
                const cf32 u = x[i + j];
                const cf32 v = cmul(x[i + j + half], twiddle_[j * tw_stride]);
                x[i + j] = {u.re + v.re, u.im + v.im};
                x[i + j + half] = {u.re - v.re, u.im - v.im};
            }
        }
    } else if (job_step_ == kStepAccumulate) {
        for (int i = 0; i < kFftN; i++) {
            // store in shifted order: index kFftN/2 is 0 Hz
            const float p = mag2(x[(i + kFftN / 2) & (kFftN - 1)]);
            spec_[i] += kSpecAlpha * (p - spec_[i]);
        }
    } else if (job_step_ == kStepComb) {
        comb_search();
    } else {
        refine_tones();
        job_step_ = -1;
        return;
    }
    job_step_++;
}

void Demod4FSK::comb_search() {
    const float sp = tone_spacing_hz_ / kBinHz;
    const int lo = (int)((float)(kFftN / 2) - kSearchHz / kBinHz);
    const int hi = (int)((float)(kFftN / 2) + kSearchHz / kBinHz - (kTones - 1) * sp);
    float best = -1.0f;
    comb_best_ = lo;
    for (int b = lo; b <= hi; b++) {
        float m = 0.0f;
        for (int k = 0; k < kTones; k++) m += spec_at(spec_, (float)b + k * sp);
        if (m > best) {
            best = m;
            comb_best_ = b;
        }
    }
}

void Demod4FSK::refine_tones() {
    const float sp = tone_spacing_hz_ / kBinHz;
    const int best_b = comb_best_;

    // refine each tone on its own (uneven spacing) and measure the noise floor around them
    float peak_bin[kTones];
    float tone_pwr = 0.0f;
    float excl = 0.0f;
    int excl_n = 0;
    for (int k = 0; k < kTones; k++) {
        const int c = (int)lroundf((float)best_b + k * sp);
        int m = c;
        for (int i = c - 2; i <= c + 2; i++) {
            if (i > 0 && i < kFftN - 1 && spec_[i] > spec_[m]) m = i;
        }
        float d = 0.0f;
        if (m > 0 && m < kFftN - 1) {
            const float a = spec_[m - 1], b = spec_[m], e = spec_[m + 1];
            const float den = a - 2.0f * b + e;
            if (den < 0.0f) d = 0.5f * (a - e) / den;
        }
        peak_bin[k] = (float)m + d;
        tone_pwr += spec_[m];
        for (int i = c - 3; i <= c + 3; i++) {
            if (i >= 0 && i < kFftN) {
                excl += spec_[i];
                excl_n++;
            }
        }
    }
    float total = 0.0f;
    for (int i = 0; i < kFftN; i++) total += spec_[i];
    const float noise = (total - excl) / (float)(kFftN - excl_n) + 1e-20f;
    const float ratio = (tone_pwr / kTones) / noise;
    status_.signal_db = 10.0f * log10f(ratio);

    if (ratio < kDetectRatio) return;  // keep the last good estimate while the signal is absent

    for (int k = 0; k < kTones; k++) {
        const float hz = (peak_bin[k] - (float)(kFftN / 2)) * kBinHz;
        if (!status_.tones_valid || fabsf(hz - status_.tone_hz[k]) > 3.0f * kBinHz) {
            set_tone(k, hz);
        } else {
            set_tone(k, status_.tone_hz[k] + 0.3f * (hz - status_.tone_hz[k]));
        }
    }
    status_.tones_valid = true;
}

int Demod4FSK::process(cf32 x) {
    fft_in_[fft_fill_++] = x;
    if (fft_fill_ == kFftN) {
        fft_fill_ = 0;
        // Hand a windowed copy to the background job; if the previous one is somehow
        // still running, skip this block rather than stall the real-time path.
        if (job_step_ < 0) {
            for (int i = 0; i < kFftN; i++) fft_work_[i] = {fft_in_[i].re * window_[i], fft_in_[i].im * window_[i]};
            job_step_ = 0;
        }
    }

    float e[kTones];
    for (int k = 0; k < kTones; k++) {
        const cf32 p = cmul(x, osc_[k]);
        osc_[k] = cmul(osc_[k], step_[k]);
        const cf32 old = ring_[k][ring_pos_];
        ring_[k][ring_pos_] = p;
        acc_[k].re += p.re - old.re;
        acc_[k].im += p.im - old.im;
        e[k] = mag2(acc_[k]);
    }
    if (++ring_pos_ == kTs) {
        ring_pos_ = 0;
        // keep the oscillators on the unit circle and clear rounding drift in the sums
        for (int k = 0; k < kTones; k++) {
            const float n = 1.0f / sqrtf(mag2(osc_[k]));
            osc_[k].re *= n;
            osc_[k].im *= n;
            cf32 s = {0.0f, 0.0f};
            for (int i = 0; i < kTs; i++) {
                s.re += ring_[k][i].re;
                s.im += ring_[k][i].im;
            }
            acc_[k] = s;
        }
    }

    int best = 0;
    float total = e[0];
    for (int k = 1; k < kTones; k++) {
        total += e[k];
        if (e[k] > e[best]) best = k;
    }
    metric_[hist_pos_] = e[best];
    sym_[hist_pos_] = (uint8_t)best;
    e_best_[hist_pos_] = e[best];
    e_rest_[hist_pos_] = (total - e[best]) / (kTones - 1);
    if (++hist_pos_ == kTs) hist_pos_ = 0;

    if (--samples_to_eval_ > 0) return -1;
    return decide();
}

int Demod4FSK::decide() {
    // history: index j (0 = oldest) lives at (hist_pos_ + j) % kTs
    int jmax = 0;
    float mx = -1.0f, mn = 3.4e38f;
    for (int j = 0; j < kTs; j++) {
        const float m = metric_[(hist_pos_ + j) % kTs];
        if (m > mx) {
            mx = m;
            jmax = j;
        }
        if (m < mn) mn = m;
    }
    // only steer the clock when the window holds a transition (a clear peak)
    if (mx > 0.0f && (mx - mn) > 0.3f * mx) {
        float err = (float)jmax - tpos_;
        if (err >= 0.5f * kTs) err -= kTs;
        if (err < -0.5f * kTs) err += kTs;
        tpos_ += kTimingGain * err;
        if (tpos_ < 0.0f) tpos_ = 0.0f;
        if (tpos_ > kTs - 1) tpos_ = kTs - 1;
    }
    int d = (int)(tpos_ + 0.5f);
    if (d > kTs - 1) d = kTs - 1;
    const int idx = (hist_pos_ + d) % kTs;

    snr_sig_ += kSnrAlpha * (e_best_[idx] - snr_sig_);
    snr_noise_ += kSnrAlpha * (e_rest_[idx] - snr_noise_);
    status_.snr_db = 10.0f * log10f((snr_sig_ + 1e-20f) / (snr_noise_ + 1e-20f));

    // keep the decision point centred in the window; this absorbs clock drift
    const int shift = (int)lroundf(tpos_ - 0.5f * kTs);
    samples_to_eval_ = kTs + shift;
    tpos_ -= (float)shift;
    return sym_[idx];
}

}  // namespace horus
