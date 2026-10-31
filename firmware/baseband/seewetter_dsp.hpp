/*
 * Copyright (C) 2026 justin080
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

/*
 * Seewetter RTTY demodulator core (portable, no PortaPack dependencies, so it
 * can be unit tested on a PC).
 *
 * Input : complex baseband at 24 kHz, the FSK signal centred around 0 Hz
 *         (receiver tuned to the published centre frequency, e.g. DWD 4583 kHz).
 * Chain : 48 tap FIR, decimate by 3 -> 8 kHz
 *         AFC (NCO) -> two non-coherent matched filters (boxcar, one bit long)
 *         on +shift/2 and -shift/2 -> normalised discriminator
 *         -> two async UARTs (normal / inverted polarity), 1 start, 5 data, 1.5 stop
 *         256 point FFT spectrum -> AFC estimate (two-tone template) + tuning display
 * Output: 5 bit ITA2 codes, status (AFC offset, quality, polarity, spectrum),
 *         24 kHz audio (tones around 1500 Hz, like listening in USB).
 */

#ifndef __SEEWETTER_DSP_HPP__
#define __SEEWETTER_DSP_HPP__

#include <cstdint>
#include <cstddef>
#include <cmath>

namespace seewetter {

struct cf {
    float re;
    float im;
};

static inline cf cmul(const cf a, const cf b) {
    return {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
}
static inline cf cmulc(const cf a, const cf b) {  // a * conj(b)
    return {a.re * b.re + a.im * b.im, a.im * b.re - a.re * b.im};
}
static inline float cmag2(const cf a) {
    return a.re * a.re + a.im * a.im;
}

constexpr float SW_PI = 3.14159265358979f;

class Demod {
   public:
    static constexpr uint32_t FS_IN = 24000;
    static constexpr uint32_t DECIM = 3;
    static constexpr uint32_t FS = FS_IN / DECIM;  // 8000
    static constexpr size_t DEC_TAPS = 48;
    static constexpr size_t MAX_BIT_LEN = 192;  // 45.45 Bd -> 176 samples
    static constexpr size_t FFT_N = 256;
    static constexpr size_t FFT_LOG2 = 8;
    static constexpr float BIN_HZ = (float)FS / (float)FFT_N;  // 31.25 Hz
    static constexpr size_t SPEC_BINS = 80;                    // shown: +-1250 Hz
    static constexpr size_t MAX_CHARS = 48;
    static constexpr float AUDIO_TONE_HZ = 1500.0f;
    static constexpr float AFC_RANGE_HZ = 1250.0f;

    enum Polarity : uint8_t {
        POL_AUTO = 0,
        POL_NORMAL = 1,  // mark = higher frequency
        POL_INVERTED = 2
    };

    /* ---- outputs, read by the owner after process() ---- */
    uint8_t chars[MAX_CHARS]{};  // 5 bit ITA2 codes
    size_t char_count{0};

    bool spectrum_ready{false};
    uint8_t spectrum[SPEC_BINS]{};  // (dB above noise + 3) * 6, clamped

    float afc_hz{0.0f};     // currently applied correction (signal centre offset)
    float target_hz{0.0f};  // last measured signal centre (also when AFC is off)
    float quality{0.0f};    // 0..1, mean |soft decision| at the sampling points
    uint8_t active_pol{POL_NORMAL};
    bool locked{false};  // a two tone signal with the configured shift is visible
    bool squelch_open{false};

    void configure(uint32_t baud_x100, uint32_t shift_hz, uint8_t polarity, bool afc, uint8_t squelch) {
        if (baud_x100 < 4000) baud_x100 = 4000;
        if (baud_x100 > 30000) baud_x100 = 30000;
        bit_len_ = (float)FS * 100.0f / (float)baud_x100;
        box_len_ = (size_t)(bit_len_ + 0.5f);
        if (box_len_ > MAX_BIT_LEN) box_len_ = MAX_BIT_LEN;
        if (box_len_ < 8) box_len_ = 8;
        shift_hz_ = (float)shift_hz;
        att_ = 8.0f / bit_len_;            // attack: about 1/8 bit
        dec_ = 1.0f / (12.0f * bit_len_);  // release: about 1.5 characters
        polarity_ = polarity;
        afc_enabled_ = afc;
        static constexpr float sq_table[4] = {0.0f, 0.30f, 0.42f, 0.55f};
        squelch_level_ = sq_table[squelch & 3];

        // Decimation filter: Blackman windowed sinc, cut-off 2.0 kHz @ 24 kHz.
        const float fc = 2000.0f / (float)FS_IN;
        float sum = 0.0f;
        for (size_t i = 0; i < DEC_TAPS; i++) {
            const float m = (float)i - (float)(DEC_TAPS - 1) / 2.0f;
            const float x = 2.0f * SW_PI * fc * m;
            const float sinc = (std::fabs(m) < 1e-6f) ? 2.0f * fc : std::sin(x) / (SW_PI * m);
            const float w = 0.42f - 0.5f * std::cos(2.0f * SW_PI * i / (DEC_TAPS - 1)) +
                            0.08f * std::cos(4.0f * SW_PI * i / (DEC_TAPS - 1));
            dec_taps_[i] = sinc * w;
            sum += dec_taps_[i];
        }
        for (size_t i = 0; i < DEC_TAPS; i++) dec_taps_[i] /= sum;

        // FFT tables
        for (size_t i = 0; i < FFT_N; i++)
            hann_[i] = 0.5f - 0.5f * std::cos(2.0f * SW_PI * i / FFT_N);
        for (size_t i = 0; i < FFT_N / 2; i++)
            twiddle_[i] = {std::cos(2.0f * SW_PI * i / FFT_N), -std::sin(2.0f * SW_PI * i / FFT_N)};

        set_tone_rotators();
        reset();
    }

    void reset() {
        for (auto& v : dec_line_) v = {0, 0};
        dec_pos_ = 0;
        dec_phase_ = 0;
        for (auto& v : buf_hi_) v = {0, 0};
        for (auto& v : buf_lo_) v = {0, 0};
        sum_hi_ = sum_lo_ = {0, 0};
        box_pos_ = 0;
        refresh_counter_ = 0;
        p_afc_ = {1, 0};
        p_tone_ = {1, 0};
        p_audio_ = {1, 0};
        for (auto& v : spec_avg_) v = 0.0f;
        for (auto& v : afc_avg_) v = 0.0f;
        acquired_ = false;
        jump_votes_ = 0;
        jump_cand_ = 0.0f;
        unlocked_count_ = 0;
        fft_pos_ = 0;
        frames_ = 0;
        uart_[0] = Uart{};
        uart_[1] = Uart{};
        active_ = (polarity_ == POL_INVERTED) ? 1 : 0;
        active_pol = active_ ? POL_INVERTED : POL_NORMAL;
        env_hi_ = env_lo_ = 0.0f;
        q_ema_ = 0.0f;
        quality = 0.0f;
        afc_hz = 0.0f;
        target_hz = 0.0f;
        locked = false;
        squelch_open = (squelch_level_ <= 0.0f);
        agc_ = 1e-3f;
        audio_prev_ = audio_cur_ = 0.0f;
        set_afc_rotator();
    }

    /* Feed one complex sample at 24 kHz, returns one audio sample at 24 kHz (about +-1). */
    float process(const cf x) {
        // Decimation FIR: delay line stored twice, so the taps always see a contiguous window.
        dec_line_[dec_pos_] = x;
        dec_line_[dec_pos_ + DEC_TAPS] = x;
        if (++dec_pos_ >= DEC_TAPS) dec_pos_ = 0;

        if (++dec_phase_ >= DECIM) {
            dec_phase_ = 0;
            cf acc{0, 0};
            const cf* w = &dec_line_[dec_pos_];  // oldest sample first
            for (size_t i = 0; i < DEC_TAPS; i++) {
                acc.re += w[i].re * dec_taps_[i];
                acc.im += w[i].im * dec_taps_[i];
            }
            audio_prev_ = audio_cur_;
            audio_cur_ = step8k(acc);
        }
        // Linear interpolation 8 kHz -> 24 kHz (images at 6.5 kHz and up are about 30 dB down).
        const float frac = (float)dec_phase_ / (float)DECIM;
        return audio_prev_ + (audio_cur_ - audio_prev_) * frac;
    }

    /* Owner acknowledges the outputs. */
    void clear_chars() { char_count = 0; }

   private:
    struct Uart {
        enum State : uint8_t { WAIT_MARK,
                               WAIT_EDGE,
                               RECV };
        State st{WAIT_MARK};
        float prev{0};
        float t{0};
        float next{0};
        uint8_t bit{0};
        uint8_t code{0};
        float conf{0};
        float score{0};

        /* Returns true when a character frame is complete. */
        bool step(const float v, const float T, uint8_t& out_code, bool& out_ok, float& out_q) {
            bool done = false;
            switch (st) {
                case WAIT_MARK:
                    if (v > 0.2f) st = WAIT_EDGE;
                    break;
                case WAIT_EDGE:
                    if (v < 0.0f && prev >= 0.0f) {
                        // Mark -> space transition: start bit. Interpolate the zero crossing.
                        const float d = prev - v;
                        t = (d > 1e-9f) ? 1.0f - prev / d : 0.5f;
                        next = 0.5f * T;  // centre of the start bit (filter delay already included)
                        bit = 0;
                        code = 0;
                        conf = 0;
                        st = RECV;
                    }
                    break;
                case RECV:
                    t += 1.0f;
                    if (t >= next) {
                        if (bit == 0) {
                            if (v >= 0.0f) {  // glitch, not a start bit
                                st = WAIT_EDGE;
                                break;
                            }
                            conf -= v;
                        } else if (bit <= 5) {
                            if (v > 0.0f) code |= (uint8_t)(1u << (bit - 1));
                            conf += std::fabs(v);
                        } else {
                            out_ok = v > 0.0f;  // stop bit must be mark
                            conf += std::fabs(v);
                            out_q = conf / 7.0f;
                            out_code = code;
                            score = score * 0.85f + (out_ok ? 0.15f * out_q : 0.0f);
                            st = out_ok ? WAIT_EDGE : WAIT_MARK;
                            done = true;
                            break;
                        }
                        bit++;
                        next += T;
                    }
                    break;
            }
            prev = v;
            return done;
        }
    };

    // configuration
    float bit_len_{160.0f};
    size_t box_len_{160};
    float shift_hz_{450.0f};
    uint8_t polarity_{POL_AUTO};
    bool afc_enabled_{true};
    float squelch_level_{0.0f};

    // decimator
    float dec_taps_[DEC_TAPS]{};
    cf dec_line_[DEC_TAPS * 2]{};
    size_t dec_pos_{0};
    uint32_t dec_phase_{0};

    // NCOs
    cf p_afc_{1, 0}, rot_afc_{1, 0};
    cf p_tone_{1, 0}, rot_tone_{1, 0};
    cf p_audio_{1, 0}, rot_audio_{1, 0};

    // matched filters
    cf buf_hi_[MAX_BIT_LEN]{};
    cf buf_lo_[MAX_BIT_LEN]{};
    cf sum_hi_{0, 0}, sum_lo_{0, 0};
    size_t box_pos_{0};
    uint32_t refresh_counter_{0};

    // ATC envelope trackers
    float env_hi_{0}, env_lo_{0};
    float att_{0.1f}, dec_{1.0f / 3600.0f};

    // UARTs: [0] normal (mark = high tone), [1] inverted
    Uart uart_[2]{};
    uint8_t active_{0};
    float q_ema_{0};

    // audio
    float agc_{1e-3f};
    float audio_prev_{0}, audio_cur_{0};

    // spectrum / AFC
    float hann_[FFT_N]{};
    cf twiddle_[FFT_N / 2]{};
    cf fft_in_[FFT_N]{};
    cf fft_work_[FFT_N]{};
    float spec_avg_[FFT_N]{};  // index 0 = -FS/2 (fft shifted), display average
    float afc_avg_[FFT_N]{};   // slower average for the AFC
    bool acquired_{false};     // AFC has a valid position
    uint8_t jump_votes_{0};
    float jump_cand_{0.0f};
    uint8_t unlocked_count_{0};
    float scratch_[FFT_N]{};
    size_t fft_pos_{0};
    uint32_t frames_{0};

    static cf rotator(const float hz) {
        const float w = -2.0f * SW_PI * hz / (float)FS;
        return {std::cos(w), std::sin(w)};
    }
    static void renorm(cf& p) {
        const float g = 1.5f - 0.5f * cmag2(p);
        p.re *= g;
        p.im *= g;
    }
    void set_tone_rotators() {
        rot_tone_ = rotator(shift_hz_ * 0.5f);  // brings +shift/2 down to 0 Hz
        rot_audio_ = rotator(-AUDIO_TONE_HZ);   // moves the signal up to 1500 Hz
    }
    void set_afc_rotator() {
        rot_afc_ = rotator(afc_hz);
    }

    float step8k(const cf x) {
        // ---- spectrum capture (before AFC, so the estimate is absolute) ----
        fft_in_[fft_pos_] = x;
        if (++fft_pos_ >= FFT_N) {
            fft_pos_ = 0;
            do_spectrum();
        }

        // ---- AFC mix ----
        const cf y = cmul(x, p_afc_);
        p_afc_ = cmul(p_afc_, rot_afc_);

        // ---- audio (AGC'd USB-like tones around 1500 Hz) ----
        const float m = std::fabs(y.re) + std::fabs(y.im);
        agc_ = (m > agc_) ? agc_ + (m - agc_) * 0.05f : agc_ * 0.9995f;
        if (agc_ < 1e-9f) agc_ = 1e-9f;
        const float audio = 0.35f * cmul(y, p_audio_).re / agc_;
        p_audio_ = cmul(p_audio_, rot_audio_);

        // ---- matched filters ----
        const cf z_hi = cmul(y, p_tone_);
        const cf z_lo = cmulc(y, p_tone_);
        p_tone_ = cmul(p_tone_, rot_tone_);

        sum_hi_.re += z_hi.re - buf_hi_[box_pos_].re;
        sum_hi_.im += z_hi.im - buf_hi_[box_pos_].im;
        sum_lo_.re += z_lo.re - buf_lo_[box_pos_].re;
        sum_lo_.im += z_lo.im - buf_lo_[box_pos_].im;
        buf_hi_[box_pos_] = z_hi;
        buf_lo_[box_pos_] = z_lo;
        if (++box_pos_ >= box_len_) box_pos_ = 0;

        if (++refresh_counter_ >= 4096) {
            // Re-sum from the buffers to remove accumulated float rounding, renormalise NCOs.
            refresh_counter_ = 0;
            sum_hi_ = sum_lo_ = {0, 0};
            for (size_t i = 0; i < box_len_; i++) {
                sum_hi_.re += buf_hi_[i].re;
                sum_hi_.im += buf_hi_[i].im;
                sum_lo_.re += buf_lo_[i].re;
                sum_lo_.im += buf_lo_[i].im;
            }
        }
        if ((refresh_counter_ & 63) == 0) {
            renorm(p_afc_);
            renorm(p_tone_);
            renorm(p_audio_);
        }

        const float a_hi = std::sqrt(cmag2(sum_hi_));
        const float a_lo = std::sqrt(cmag2(sum_lo_));

        // ATC: scale each tone by its own envelope, so a tone that fades (selective
        // fading on HF) does not shift the decision threshold or the zero crossing
        // used for bit timing. The ratio keeps the decision robust in noise.
        track(a_hi, env_hi_);
        track(a_lo, env_lo_);
        const float m_hi = a_hi / (env_hi_ + 1e-20f);
        const float m_lo = a_lo / (env_lo_ + 1e-20f);
        float d = (m_hi - m_lo) / (m_hi + m_lo + 1e-20f);  // +1 = high tone, -1 = low tone
        if (d > 1.0f) d = 1.0f;
        if (d < -1.0f) d = -1.0f;

        // ---- UARTs ----
        for (uint8_t u = 0; u < 2; u++) {
            uint8_t code = 0;
            bool ok = false;
            float q = 0.0f;
            const float v = (u == 0) ? d : -d;
            if (uart_[u].step(v, bit_len_, code, ok, q)) {
                if (polarity_ == POL_AUTO) {
                    // switch to the other decoder only when it is clearly better
                    const uint8_t other = active_ ^ 1;
                    if (uart_[other].score > uart_[active_].score + 0.12f) {
                        active_ = other;
                        q_ema_ = 0.0f;
                    }
                } else {
                    active_ = (polarity_ == POL_INVERTED) ? 1 : 0;
                }
                if (u == active_) {
                    q_ema_ = q_ema_ * 0.7f + (ok ? q : q * 0.5f) * 0.3f;
                    quality = q_ema_;
                    squelch_open = q_ema_ >= squelch_level_;
                    if (ok && squelch_open && char_count < MAX_CHARS)
                        chars[char_count++] = code;
                }
            }
        }
        active_pol = active_ ? POL_INVERTED : POL_NORMAL;
        return audio;
    }

    /* In place radix 2 FFT, input in natural order. */
    void fft(cf* d) {
        for (size_t i = 1, j = 0; i < FFT_N; i++) {
            size_t bit = FFT_N >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) {
                const cf t = d[i];
                d[i] = d[j];
                d[j] = t;
            }
        }
        for (size_t len = 2; len <= FFT_N; len <<= 1) {
            const size_t half = len >> 1;
            const size_t step = FFT_N / len;
            for (size_t i = 0; i < FFT_N; i += len) {
                for (size_t k = 0; k < half; k++) {
                    const cf w = twiddle_[k * step];
                    const cf t = cmul(d[i + k + half], w);
                    const cf u = d[i + k];
                    d[i + k] = {u.re + t.re, u.im + t.im};
                    d[i + k + half] = {u.re - t.re, u.im - t.im};
                }
            }
        }
    }

    float power_at(const float bin) const {  // bin relative to 0 Hz, fractional
        float idx = bin + (float)(FFT_N / 2);
        if (idx < 0.0f) idx = 0.0f;
        if (idx > (float)(FFT_N - 2)) idx = (float)(FFT_N - 2);
        const size_t i = (size_t)idx;
        const float f = idx - (float)i;
        return afc_avg_[i] * (1.0f - f) + afc_avg_[i + 1] * f;
    }

    void track(const float a, float& env) const {
        env += (a - env) * ((a > env) ? att_ : dec_);
    }

    float select_kth(float* a, size_t n, size_t k) {  // quickselect, destroys a
        size_t lo = 0, hi = n - 1;
        while (lo < hi) {
            const float pivot = a[(lo + hi) >> 1];
            size_t i = lo, j = hi;
            while (i <= j) {
                while (a[i] < pivot) i++;
                while (a[j] > pivot) j--;
                if (i <= j) {
                    const float t = a[i];
                    a[i] = a[j];
                    a[j] = t;
                    i++;
                    if (j == 0) break;
                    j--;
                }
            }
            if (k <= j)
                hi = j;
            else if (k >= i)
                lo = i;
            else
                break;
        }
        return a[k];
    }

    void do_spectrum() {
        for (size_t i = 0; i < FFT_N; i++) {
            fft_work_[i].re = fft_in_[i].re * hann_[i];
            fft_work_[i].im = fft_in_[i].im * hann_[i];
        }
        fft(fft_work_);
        for (size_t k = 0; k < FFT_N; k++) {
            const size_t m = (k + FFT_N / 2) & (FFT_N - 1);  // fft shift
            const float p = cmag2(fft_work_[k]);
            spec_avg_[m] += (p - spec_avg_[m]) * 0.2f;
            afc_avg_[m] += (p - afc_avg_[m]) * 0.06f;
        }
        frames_++;

        if ((frames_ & 3) != 0) return;  // every 4 frames = 128 ms

        // Noise floor: 30 % percentile of the whole 8 kHz wide spectrum.
        for (size_t i = 0; i < FFT_N; i++) scratch_[i] = spec_avg_[i];
        float noise = select_kth(scratch_, FFT_N, (FFT_N * 3) / 10);
        if (noise < 1e-20f) noise = 1e-20f;

        // Display bins around the centre.
        for (size_t i = 0; i < SPEC_BINS; i++) {
            const float p = spec_avg_[FFT_N / 2 - SPEC_BINS / 2 + i];
            float db = 10.0f * std::log10(p / noise + 1e-12f);
            float v = (db + 3.0f) * 6.0f;
            if (v < 0.0f) v = 0.0f;
            if (v > 255.0f) v = 255.0f;
            spectrum[i] = (uint8_t)v;
        }
        spectrum_ready = true;

        if ((frames_ & 7) != 0) return;  // AFC every 256 ms

        for (size_t i = 0; i < FFT_N; i++) scratch_[i] = afc_avg_[i];
        noise = select_kth(scratch_, FFT_N, (FFT_N * 3) / 10);
        if (noise < 1e-20f) noise = 1e-20f;

        // Two tone template search: maximise sqrt(P(c+h) * P(c-h)).
        const float h = shift_hz_ * 0.5f / BIN_HZ;
        float range = AFC_RANGE_HZ / BIN_HZ;
        const float max_c = (float)(FFT_N / 2 - 2) - h;
        if (range > max_c) range = max_c;
        float best_c = 0.0f, best_s = -1.0f, best_min = 0.0f;
        for (float c = -range; c <= range; c += 0.25f) {
            const float pa = power_at(c + h);
            const float pb = power_at(c - h);
            const float s = pa * pb;
            if (s > best_s) {
                best_s = s;
                best_c = c;
                best_min = pa < pb ? pa : pb;
            }
        }
        // parabolic refinement on sqrt score
        {
            const float sm = std::sqrt(power_at(best_c - 0.25f + h) * power_at(best_c - 0.25f - h));
            const float s0 = std::sqrt(best_s);
            const float sp = std::sqrt(power_at(best_c + 0.25f + h) * power_at(best_c + 0.25f - h));
            const float den = sm - 2.0f * s0 + sp;
            if (den < 0.0f) {
                float off = 0.5f * (sm - sp) / den;
                if (off > 0.5f) off = 0.5f;
                if (off < -0.5f) off = -0.5f;
                best_c += off * 0.25f;
            }
        }

        const float snr = best_min / noise;
        if (locked) {
            if (snr < 2.0f) locked = false;
        } else {
            if (snr > 4.0f) locked = true;
        }
        if (!locked) {
            // after about 10 s without a signal, acquire from scratch again
            if (unlocked_count_ < 255) unlocked_count_++;
            if (unlocked_count_ > 40) acquired_ = false;
            return;
        }
        unlocked_count_ = 0;

        const float cand_hz = best_c * BIN_HZ;
        target_hz = cand_hz;
        if (!afc_enabled_) return;

        if (!acquired_) {
            afc_hz = cand_hz;
            acquired_ = true;
            jump_votes_ = 0;
        } else {
            const float err = cand_hz - afc_hz;
            if (std::fabs(err) <= 60.0f) {
                afc_hz += 0.4f * err;  // fine tracking
                jump_votes_ = 0;
            } else {
                // A big jump only when the new position is clearly better than the
                // current one, three times in a row. Under selective fading one tone can
                // sink into the noise, and a template shifted by one whole shift then
                // looks almost as good as the right one.
                const float cur_c = afc_hz / BIN_HZ;
                const float cur_s = power_at(cur_c + h) * power_at(cur_c - h);
                const bool better = best_s > 4.0f * cur_s;
                if (better && jump_votes_ > 0 && std::fabs(cand_hz - jump_cand_) < 60.0f) {
                    if (++jump_votes_ >= 3) {
                        afc_hz = cand_hz;
                        jump_votes_ = 0;
                    }
                } else {
                    jump_votes_ = better ? 1 : 0;
                }
                jump_cand_ = cand_hz;
            }
        }
        set_afc_rotator();
    }
};

}  // namespace seewetter

#endif /*__SEEWETTER_DSP_HPP__*/
