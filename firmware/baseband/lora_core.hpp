/*
 * LoRa receiver core (explicit header, BW-limited input with 2 samples per chip).
 * Portable C++ (no Mayhem dependency) so it can be tested on a PC with real captures.
 *
 * Input: complex samples at 2 x BW (e.g. 125 kS/s for BW 62.5 kHz), signal centred at 0 Hz.
 * Output: decoded packets (payload bytes, CRC status) through a callback.
 *
 * Chain (validated on MeshCore captures, SF8 / 62.5 kHz / CR 4/8 / sync word 0x12):
 *   dechirp + FFT (2N points, bins k and k-N folded) -> preamble detection -> CFO / timing from
 *   the preamble and SFD peaks -> symbols (per-symbol frequency tracking) -> Gray -> deinterleave ->
 *   Hamming -> dewhitening -> CRC-16.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __LORA_CORE_H__
#define __LORA_CORE_H__

#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>

namespace lora {

constexpr int OS = 2;      // samples per chip
constexpr int MAX_SF = 8;  // RAM budget of the M4 (SF7 / SF8)
constexpr int MAX_N = 1 << MAX_SF;
constexpr int MAX_L = MAX_N * OS;
constexpr int RING_SYMBOLS = 8;

// sin/cos without libm (range reduction to [-pi, pi], then odd/even polynomials, error < 2e-6)
inline void sincos_fast(float x, float& s, float& c) {
    const float pi = 3.14159265f, two_pi = 6.28318531f;
    x -= two_pi * (float)(int)(x / two_pi);
    if (x > pi) x -= two_pi;
    if (x < -pi) x += two_pi;
    // reduce to [-pi/2, pi/2] keeping the cosine sign
    float sign_c = 1;
    if (x > pi / 2) {
        x = pi - x;
        sign_c = -1;
    } else if (x < -pi / 2) {
        x = -pi - x;
        sign_c = -1;
    }
    float x2 = x * x;
    s = x * (1 - x2 / 6 * (1 - x2 / 20 * (1 - x2 / 42 * (1 - x2 / 72 * (1 - x2 / 110)))));
    c = sign_c * (1 - x2 / 2 * (1 - x2 / 12 * (1 - x2 / 30 * (1 - x2 / 56 * (1 - x2 / 90 * (1 - x2 / 132))))));
}

struct Packet {
    uint8_t length{0};
    uint8_t cr{0};
    bool has_crc{false};
    bool crc_ok{false};
    float cfo_bins{0};
    float q_mean{0};  // mean peak/noise ratio of the symbols (SNR in dB = 20 log10)
    uint8_t data[255]{};
};

class Receiver {
   public:
    using Callback = void (*)(const Packet&, void* ctx);

    void configure(int sf, uint8_t sync_word, Callback cb, void* ctx) {
        sf_ = sf < 7 ? 7 : (sf > MAX_SF ? MAX_SF : sf);
        n_ = 1 << sf_;
        l_ = n_ * OS;
        sync_word_ = sync_word;
        cb_ = cb;
        ctx_ = ctx;
        // chirp and FFT tables
        const float pi = 3.14159265f;
        for (int m = 0; m < l_; m++) {
            float t = (float)m / OS;
            float s, c;
            sincos_fast(pi * (t * t / n_ - t), s, c);
            up_[m] = {c, s};
        }
        for (int m = 0; m < l_ / 2; m++) {
            float s, c;
            sincos_fast(-2.0f * pi * m / l_, s, c);
            tw_[m] = {c, s};
        }
        bits_ = 0;
        while ((1 << bits_) < l_) bits_++;
        reset();
    }

    void reset() {
        state_ = State::Search;
        run_ = 0;
        next_ = written_;
    }

    // Producer side (DMA thread): stores samples only. Returns true when a window is ready.
    template <typename C>
    bool push(const C* x, int count) {
        uint32_t w = written_;
        for (int i = 0; i < count; i++) {
            ring_[w % RING] = {(int16_t)x[i].real(), (int16_t)x[i].imag()};
            w++;
        }
        __asm__ volatile("" ::: "memory");
        written_ = w;
        return (int32_t)(w - next_) >= l_;
    }

    // Consumer side (worker thread): runs the state machine on every complete window.
    void process() {
        while (true) {
            const uint32_t w = written_;
            __asm__ volatile("" ::: "memory");
            const int32_t avail = (int32_t)(w - next_);
            if (avail < l_) break;
            if (avail > RING - 3 * MAX_L) {  // fell behind: the ring was overwritten
                // counted only when it costs a packet (while searching, the skipped samples
                // are noise or the start of a preamble that is still long enough to catch)
                if (state_ != State::Search) n_ovr++;
                state_ = State::Search;
                run_ = 0;
                next_ = w - l_;
            }
            step();
        }
    }

    // true from the preamble to the end of a packet (the gain must not rise meanwhile)
    bool busy() const { return state_ != State::Search; }
    // Detection threshold on the symbol quality (FFT peak / mean of the other bins).
    void set_threshold(float q) { thr_ = q; }
    // counters for the status line (wrap at 256): ring overruns, preambles, syncs, valid headers
    uint8_t n_ovr{0}, n_pre{0}, n_sync{0}, n_hdr{0};

   private:
    enum class State { Search,
                       Preamble,
                       Header,
                       Payload };
    static constexpr int RING = MAX_L * RING_SYMBOLS;

    int sf_{8}, n_{256}, l_{512}, bits_{9};
    uint8_t sync_word_{0x12};
    Callback cb_{nullptr};
    void* ctx_{nullptr};

    std::complex<float> up_[MAX_L];
    std::complex<float> tw_[MAX_L / 2];
    std::complex<float> buf_[MAX_L];
    float mag_[MAX_N];
    std::complex<int16_t> ring_[RING];
    volatile uint32_t written_{0};
    uint32_t next_{0};

    State state_{State::Search};
    int run_{0};
    float last_bin_{0};
    float pre_sum_{0};
    int pre_count_{0};
    int after_pre_{0};
    int miss_{0};
    float thr_{4};  // simulated: ~-8 dB SNR sensitivity, no false preamble in 300 s of noise
    bool sfd_seen_{false};
    float cfo_{0};
    float noise_{1}, signal_{0};

    // payload decoding
    int nsym_{0};
    uint16_t syms_[8];
    int hdr_raw_[8];
    int cw_len_{8}, cr_{4};
    int total_nibbles_{0}, have_nibbles_{0};
    uint8_t nibbles_[2 * 258 + 8];
    Packet pkt_{};

    // ---------------------------------------------------------------- DSP
    // |z| within 4 % (alpha max + beta min), no square root
    static inline float fmag(const std::complex<float>& z) {
        float a = fabsf(z.real()), b = fabsf(z.imag());
        return a > b ? a + 0.40f * b : b + 0.40f * a;
    }

    void fft() {
        // iterative radix-2, in place on buf_
        for (int i = 1, j = 0; i < l_; i++) {
            int bit = l_ >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(buf_[i], buf_[j]);
        }
        for (int len = 2; len <= l_; len <<= 1) {
            int step = l_ / len;
            for (int i = 0; i < l_; i += len)
                for (int k = 0; k < len / 2; k++) {
                    std::complex<float> u = buf_[i + k], v = buf_[i + k + len / 2] * tw_[k * step];
                    buf_[i + k] = u + v;
                    buf_[i + k + len / 2] = u - v;
                }
        }
    }

    // Dechirps the window at absolute position pos (down=true: against a downchirp, for upchirps),
    // removes cfo (bins), FFT, folds into mag_. Returns the peak bin with fractional offset.
    float dechirp(uint32_t pos, bool down, float cfo, float* quality) {
        float is, ic;
        sincos_fast(-2.0f * 3.14159265f * cfo / (n_ * OS), is, ic);
        std::complex<float> rot{1, 0}, inc{ic, is};
        for (int m = 0; m < l_; m++) {
            std::complex<float> c = down ? std::conj(up_[m]) : up_[m];
            std::complex<int16_t> s = ring_[(pos + m) % RING];
            buf_[m] = std::complex<float>(s.real(), s.imag()) * c * rot;
            rot *= inc;
        }
        fft();
        int k = 0;
        float sum = 0;
        for (int i = 0; i < n_; i++) {
            mag_[i] = fmag(buf_[i]) + fmag(buf_[l_ - n_ + i]);
            sum += mag_[i];
            if (mag_[i] > mag_[k]) k = i;
        }
        float mean = (sum - mag_[k]) / (n_ - 1) + 1e-9f;
        if (quality) *quality = mag_[k] / mean;
        signal_ = mag_[k];
        noise_ = mean;
        // parabolic interpolation (circular)
        float a = mag_[(k + n_ - 1) % n_], b = mag_[k], c = mag_[(k + 1) % n_];
        float d = a - 2 * b + c;
        float frac = d != 0 ? 0.5f * (a - c) / d : 0;
        return k + frac;
    }

    static float wrap(float b, int n) {  // to [-n/2, n/2)
        while (b >= n / 2) b -= n;
        while (b < -n / 2) b += n;
        return b;
    }

    // ---------------------------------------------------------------- state machine
    void step() {
        switch (state_) {
            case State::Search:
            case State::Preamble:
                search();
                break;
            case State::Header:
            case State::Payload:
                payload_symbol();
                break;
        }
    }

    void search() {
        float q;
        float b = dechirp(next_, true, 0, &q);
        if (state_ == State::Search) {
            bool same = q > thr_ && run_ > 0 && fabsf(wrap(b - last_bin_, n_)) < 1.5f;
            if (q > thr_ && (same || run_ == 0)) {
                if (!same) {
                    run_ = 0;
                    pre_sum_ = 0;
                    pre_count_ = 0;
                }
                run_++;
                // average the preamble bin (unwrapped around the first value)
                float ref = pre_count_ ? pre_sum_ / pre_count_ : b;
                pre_sum_ += ref + wrap(b - ref, n_);
                pre_count_++;
                last_bin_ = b;
                if (run_ >= 6) {
                    n_pre++;
                    state_ = State::Preamble;
                    after_pre_ = 0;
                    miss_ = 0;
                }
            } else {
                run_ = 0;
            }
            next_ += l_;
            return;
        }
        // Preamble: wait for the downchirps (SFD)
        float qu;
        float bu = b;
        float bd = dechirp(next_, false, 0, &qu);
        after_pre_++;
        if (fabsf(wrap(bu - last_bin_, n_)) < 1.5f && q > thr_) {
            // still preamble: keep averaging
            float ref = pre_sum_ / pre_count_;
            pre_sum_ += ref + wrap(bu - ref, n_);
            pre_count_++;
            miss_ = 0;
        } else {
            miss_++;
        }
        if (qu > thr_ && qu > 2 * q) {
            if (!sfd_seen_) {
                sfd_seen_ = true;  // this window may overlap the last sync symbol: use the next one
                next_ += l_;
                return;
            }
            sfd_seen_ = false;
            sync(pre_sum_ / pre_count_, bd);
            return;
        }
        sfd_seen_ = false;
        // No SFD: give up. The preamble is followed by 2 sync symbols, then the SFD, so more than
        // 4 windows without the preamble tone means a false detection (frees the receiver early).
        if (after_pre_ > 80 || miss_ > 4) {
            state_ = State::Search;
            run_ = 0;
        }
        next_ += l_;
    }

    // b_up = f + tau, b_down = f - tau (bins, mod n): f = CFO, tau = timing (chips)
    void sync(float b_up, float b_down) {
        float f = wrap((b_up + b_down) / 2, n_);
        float tau = (b_up - b_down) / 2;
        if (fabsf(f) > n_ / 4) {  // other solution of the N/2 ambiguity
            f = wrap(f + n_ / 2, n_);
            tau += n_ / 2;
        }
        tau = wrap(tau, n_);
        // aligned start of this downchirp window: the chirp started tau chips before the window start
        int32_t shift = (int32_t)lroundf(tau * OS);
        uint32_t down_pos = next_ - shift;
        // the window before may also be a downchirp (we may be on the 2nd one)
        float q1, q0;
        float bb1 = dechirp(down_pos - l_, false, f, &q1);
        dechirp(down_pos - l_, true, f, &q0);
        if (q1 > thr_ && q1 > 2 * q0 && fabsf(wrap(bb1, n_)) < 2) down_pos -= l_;
        cfo_ = f;
        n_sync++;
        next_ = down_pos + 2 * l_ + l_ / 4;  // data start (2.25 downchirps)
        state_ = State::Header;
        nsym_ = 0;
        have_nibbles_ = 0;
        total_nibbles_ = 0;
        pkt_ = Packet{};
        pkt_.cfo_bins = f;
    }

    static uint16_t gray(uint16_t x) { return x ^ (x >> 1); }

    // nearest valid codeword (data bits LSB first, then parities)
    static uint8_t hamming(const uint8_t* w, int cr) {
        int best = 0, bestd = 99;
        for (int v = 0; v < 16; v++) {
            uint8_t d0 = v & 1, d1 = v >> 1 & 1, d2 = v >> 2 & 1, d3 = v >> 3 & 1;
            uint8_t c[8] = {d0, d1, d2, d3, 0, 0, 0, 0};
            if (cr == 1) {
                c[4] = d0 ^ d1 ^ d2 ^ d3;
            } else {
                uint8_t p[4] = {(uint8_t)(d0 ^ d1 ^ d2), (uint8_t)(d1 ^ d2 ^ d3), (uint8_t)(d0 ^ d1 ^ d3), (uint8_t)(d0 ^ d2 ^ d3)};
                for (int i = 0; i < cr; i++) c[4 + i] = p[i];
            }
            int dist = 0;
            for (int i = 0; i < 4 + cr; i++) dist += c[i] != w[i];
            if (dist < bestd) {
                bestd = dist;
                best = v;
            }
        }
        return best;
    }

    // deinterleave cw_len symbols of sf_app bits into sf_app nibbles
    void deinterleave(const uint16_t* s, int sf_app, int cw_len, int cr, uint8_t* out) {
        uint8_t w[12][8];
        for (int i = 0; i < cw_len; i++)
            for (int j = 0; j < sf_app; j++)
                w[((i - j - 1) % sf_app + sf_app) % sf_app][i] = (s[i] >> (sf_app - 1 - j)) & 1;
        for (int r = 0; r < sf_app; r++) out[r] = hamming(w[r], cr);
    }

    static bool header_ok(const uint8_t* n) {
        auto b = [&](int i, int k) { return (n[i] >> k) & 1; };
        int c4 = b(0, 3) ^ b(0, 2) ^ b(0, 1) ^ b(0, 0);
        int c3 = b(0, 3) ^ b(1, 3) ^ b(1, 2) ^ b(1, 1) ^ b(2, 0);
        int c2 = b(0, 2) ^ b(1, 3) ^ b(1, 0) ^ b(2, 3) ^ b(2, 1);
        int c1 = b(0, 1) ^ b(1, 2) ^ b(1, 0) ^ b(2, 2) ^ b(2, 1) ^ b(2, 0);
        int c0 = b(0, 0) ^ b(1, 1) ^ b(2, 3) ^ b(2, 2) ^ b(2, 1) ^ b(2, 0);
        int chk = ((n[3] & 1) << 4) | n[4];
        return chk == (c4 << 4 | c3 << 3 | c2 << 2 | c1 << 1 | c0);
    }

    void payload_symbol() {
        float q;
        float b = dechirp(next_, true, cfo_, &q);
        int k = (int)lroundf(b) % n_;
        // per-symbol frequency tracking from the fractional peak position
        float frac = b - lroundf(b);
        cfo_ += 0.25f * frac;
        pkt_.q_mean += q;
        next_ += l_;

        if (state_ == State::Header) {
            hdr_raw_[nsym_] = k;
            syms_[nsym_++] = 0;
            if (nsym_ < 8) return;
            // Header symbols are multiples of 4: their remainder is an integer bin offset left by the
            // CFO / timing ambiguity of the synchronisation. Majority vote, then correct everything.
            int votes[4] = {0, 0, 0, 0};
            for (int i = 0; i < 8; i++) votes[hdr_raw_[i] & 3]++;
            int r = 0;
            for (int i = 1; i < 4; i++)
                if (votes[i] > votes[r]) r = i;
            int off = r >= 2 ? r - 4 : r;  // -2..1
            cfo_ += off;
            for (int i = 0; i < 8; i++) syms_[i] = gray((uint16_t)((((hdr_raw_[i] - off) % n_ + n_) % n_) >> 2));
            uint8_t nib[12];
            deinterleave(syms_, sf_ - 2, 8, 4, nib);
            if (!header_ok(nib)) {
                reset();
                return;
            }
            n_hdr++;
            pkt_.length = nib[0] << 4 | nib[1];
            cr_ = nib[2] >> 1;
            pkt_.has_crc = nib[2] & 1;
            if (cr_ < 1 || cr_ > 4) {
                reset();
                return;
            }
            pkt_.cr = cr_;
            cw_len_ = 4 + cr_;
            total_nibbles_ = 2 * (pkt_.length + (pkt_.has_crc ? 2 : 0));
            for (int i = 5; i < sf_ - 2; i++) nibbles_[have_nibbles_++] = nib[i];
            nsym_ = 0;
            state_ = State::Payload;
            if (have_nibbles_ >= total_nibbles_) finish();
            return;
        }
        syms_[nsym_++] = gray((uint16_t)k);
        if (nsym_ < cw_len_) return;
        nsym_ = 0;
        uint8_t nib[12];
        deinterleave(syms_, sf_, cw_len_, cr_, nib);
        for (int i = 0; i < sf_ && have_nibbles_ < (int)sizeof(nibbles_); i++) nibbles_[have_nibbles_++] = nib[i];
        if (have_nibbles_ >= total_nibbles_) finish();
    }

    void finish() {
        // bytes (low nibble first), dewhitening of the payload
        uint8_t r = 0xFF;
        int len = pkt_.length;
        for (int i = 0; i < len; i++) {
            uint8_t v = nibbles_[2 * i] | nibbles_[2 * i + 1] << 4;
            pkt_.data[i] = v ^ r;
            // whitening sequence FF FE FC F8 F0 E1 C2 85 ...: one LFSR step per byte
            uint8_t fb = ((r >> 7) ^ (r >> 5) ^ (r >> 4) ^ (r >> 3)) & 1;
            r = (uint8_t)((r << 1) | fb);
        }
        if (pkt_.has_crc) {
            uint16_t rx = (nibbles_[2 * len] | nibbles_[2 * len + 1] << 4) | (uint16_t)(nibbles_[2 * len + 2] | nibbles_[2 * len + 3] << 4) << 8;
            uint16_t crc = 0;
            for (int i = 0; i + 2 < len; i++) {
                crc ^= pkt_.data[i] << 8;
                for (int b = 0; b < 8; b++) crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
            }
            if (len >= 2) crc ^= pkt_.data[len - 1] ^ (pkt_.data[len - 2] << 8);
            pkt_.crc_ok = crc == rx;
        }
        int nsyms = 8 + ((total_nibbles_ - (sf_ - 7) + sf_ - 1) / sf_) * cw_len_;
        pkt_.q_mean /= nsyms > 0 ? nsyms : 1;
        if (cb_) cb_(pkt_, ctx_);
        reset();
    }
};

}  // namespace lora

#endif
