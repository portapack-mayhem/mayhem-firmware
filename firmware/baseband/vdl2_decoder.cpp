/*
 * Copyright (C) 2026 Xmoo26
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

#include "vdl2_decoder.hpp"

#include <cmath>
#include <cstring>

namespace vdl2 {

namespace {

constexpr float PI = 3.14159265359f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float PI_4 = PI / 4.0f;

// Header: 3 reserved bits, the transmission length (17 bits, least significant first) and
// 5 FEC bits; each row is a parity check over the 25 bits as sent.
constexpr uint32_t HEADER_CHECKS[5] = {
    0b0000000011111111111110000,
    0b0011111100001111111101000,
    0b1100011100110000111100100,
    0b1101101101010011001100010,
    0b0110100111100101010100001,
};
constexpr size_t HEADER_BITS = 25;
constexpr uint32_t HEADER_MASK = (1u << 22) - 1;  // without the reserved bits
constexpr uint16_t SCRAMBLER_SEED = 0x6959;

// The training sequence after the ramp-up: phase after each symbol, in steps of pi/4.
constexpr int8_t PREAMBLE_PHASE[16] = {0, 3, -3, 1, 1, 2, 0, 4, -3, 4, -2, 3, 1, -2, -3, 0};

// Phase change in steps of pi/4 -> the three bits it carries, first bit first.
constexpr uint8_t GRAY[8] = {0, 1, 3, 2, 6, 7, 5, 4};

uint8_t parity(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return v & 1;
}

uint8_t header_syndrome(const uint32_t header) {
    uint8_t s = 0;
    for (size_t i = 0; i < 5; i++)
        s |= parity(header & HEADER_CHECKS[i]) << (4 - i);
    return s;
}

uint32_t reverse_bits(uint32_t v, const size_t bits) {
    uint32_t r = 0;
    for (size_t i = 0; i < bits; i++) {
        r = (r << 1) | (v & 1);
        v >>= 1;
    }
    return r;
}

size_t fec_octets_for(const size_t block_octets) {
    if (block_octets < 3) return 0;
    if (block_octets < 31) return 2;
    if (block_octets < 68) return 4;
    return 6;
}

// X.25 FCS over a frame including its FCS: 0xF0B8 when it is right.
uint16_t crc_x25(const uint8_t* data, const size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (size_t b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
    return crc;
}

float wrap(float a) {
    while (a > PI) a -= TWO_PI;
    while (a <= -PI) a += TWO_PI;
    return a;
}

// atan2 to within about 0.005 rad, much cheaper than atan2f.
float fast_atan2(const float y, const float x) {
    const float ax = fabsf(x), ay = fabsf(y);
    if (ax < 1e-20f && ay < 1e-20f) return 0.0f;
    const float a = (ax < ay) ? ax / ay : ay / ax;
    const float s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = PI / 2.0f - r;
    if (x < 0.0f) r = PI - r;
    return (y < 0.0f) ? -r : r;
}

}  // namespace

// --- Reed-Solomon -------------------------------------------------------------------------

ReedSolomon::ReedSolomon() {
    uint16_t x = 1;
    for (size_t i = 0; i < 255; i++) {
        exp_[i] = x;
        log_[x] = i;
        x <<= 1;
        if (x & 0x100) x ^= 0x187;
    }
    for (size_t i = 255; i < exp_.size(); i++)
        exp_[i] = exp_[i - 255];
}

uint8_t ReedSolomon::mul(const uint8_t a, const uint8_t b) const {
    return (a && b) ? exp_[log_[a] + log_[b]] : 0;
}

uint8_t ReedSolomon::div(const uint8_t a, const uint8_t b) const {
    return a ? exp_[(log_[a] + 255 - log_[b]) % 255] : 0;
}

uint8_t ReedSolomon::pow(int e) const {
    e %= 255;
    if (e < 0) e += 255;
    return exp_[e];
}

// Errors and erasures: Berlekamp-Massey started from the erasure locator, Chien search,
// Forney. Position j of the array is the coefficient of x^(254 - j), its locator alpha^(254 - j).
int ReedSolomon::decode(uint8_t* data, const uint8_t* erasures, const size_t erasure_count) const {
    uint8_t s[NROOTS];
    bool all_zero = true;
    for (size_t i = 0; i < NROOTS; i++) {
        const uint8_t root = pow(FCR + i);
        uint8_t v = 0;
        for (size_t j = 0; j < RS_N; j++)
            v = mul(v, root) ^ data[j];
        s[i] = v;
        all_zero &= v == 0;
    }
    if (all_zero)
        return 0;
    if (erasure_count > NROOTS)
        return -1;

    // Lambda(x) = prod (1 - X_e x) over the erasures, lowest power first
    uint8_t lambda[NROOTS + 1]{1};
    for (size_t e = 0; e < erasure_count; e++) {
        const uint8_t xe = pow(RS_N - 1 - erasures[e]);
        for (size_t i = e + 1; i > 0; i--)
            lambda[i] ^= mul(xe, lambda[i - 1]);
    }
    uint8_t b[NROOTS + 1];
    memcpy(b, lambda, sizeof(b));
    size_t l = erasure_count;

    for (size_t r = erasure_count + 1; r <= NROOTS; r++) {
        uint8_t delta = 0;
        for (size_t i = 0; i < r; i++)
            delta ^= mul(lambda[i], s[r - 1 - i]);
        // b <- x * b, and Lambda <- Lambda - delta * x * b_old
        uint8_t xb[NROOTS + 1]{0};
        for (size_t i = NROOTS; i > 0; i--)
            xb[i] = b[i - 1];
        if (delta == 0) {
            memcpy(b, xb, sizeof(b));
            continue;
        }
        uint8_t t[NROOTS + 1];
        for (size_t i = 0; i <= NROOTS; i++)
            t[i] = lambda[i] ^ mul(delta, xb[i]);
        if (2 * l <= r + erasure_count - 1) {
            l = r + erasure_count - l;
            for (size_t i = 0; i <= NROOTS; i++)
                b[i] = div(lambda[i], delta);
        } else {
            memcpy(b, xb, sizeof(b));
        }
        memcpy(lambda, t, sizeof(lambda));
    }

    size_t degree = 0;
    for (size_t i = 0; i <= NROOTS; i++)
        if (lambda[i]) degree = i;
    if (degree == 0 || degree > NROOTS)
        return -1;

    // Omega(x) = S(x) Lambda(x) mod x^NROOTS
    uint8_t omega[NROOTS]{0};
    for (size_t i = 0; i < NROOTS; i++)
        for (size_t j = 0; j <= i; j++)
            omega[i] ^= mul(s[i - j], lambda[j]);

    size_t found = 0;
    for (size_t j = 0; j < RS_N; j++) {
        const int power = RS_N - 1 - j;
        const uint8_t x_inv = pow(-power);
        uint8_t v = 0, xp = 1;
        for (size_t i = 0; i <= degree; i++) {
            v ^= mul(lambda[i], xp);
            xp = mul(xp, x_inv);
        }
        if (v)
            continue;
        // Forney: e = X^(1-FCR) Omega(X^-1) / Lambda'(X^-1)
        uint8_t num = 0, den = 0;
        xp = 1;
        for (size_t i = 0; i < NROOTS; i++) {
            num ^= mul(omega[i], xp);
            xp = mul(xp, x_inv);
        }
        xp = 1;
        for (size_t i = 1; i <= degree; i += 2) {
            den ^= mul(lambda[i], xp);
            xp = mul(xp, mul(x_inv, x_inv));
        }
        if (!den)
            return -1;
        data[j] ^= mul(pow(power * (1 - FCR)), div(num, den));
        found++;
    }
    return (found == degree) ? (int)found : -1;
}

// --- Burst decoder ------------------------------------------------------------------------

BurstDecoder::BurstDecoder(FrameHandler handler, void* context)
    : handler_{handler}, context_{context} {
    // Only single bit errors outside the reserved bits are corrected; other syndromes stay 0.
    for (size_t bit = 0; bit < 22; bit++)
        header_errors_[header_syndrome(1u << bit)] = 1u << bit;
}

void BurstDecoder::start() {
    state_ = State::Header;
    lfsr_ = SCRAMBLER_SEED;
    header_ = 0;
    count_ = 0;
}

bool BurstDecoder::bit(uint8_t b) {
    const uint8_t s = (lfsr_ ^ (lfsr_ >> 14)) & 1;
    lfsr_ = (lfsr_ >> 1) | (s << 14);
    b ^= s;

    if (state_ == State::Header) {
        header_ = (header_ << 1) | b;
        if (++count_ < HEADER_BITS)
            return true;
        return header_done();
    }

    buffer_[count_ >> 3] |= b << (count_ & 7);
    if (++count_ < 8 * (octets_ + fec_octets_))
        return true;
    decode_data();
    return false;
}

bool BurstDecoder::header_done() {
    uint32_t h = header_ & HEADER_MASK;  // the reserved symbol carries nothing
    const uint8_t syndrome = header_syndrome(h);
    if (syndrome) {
        if (!header_errors_[syndrome])
            return false;
        h ^= header_errors_[syndrome];
    }
    data_bits_ = reverse_bits(h >> 5, 17);
    if (data_bits_ == 0 || data_bits_ > MAX_DATA_BITS)
        return false;

    octets_ = (data_bits_ + 7) / 8;
    blocks_ = (octets_ + RS_K - 1) / RS_K;
    last_block_ = octets_ - (blocks_ - 1) * RS_K;
    last_fec_ = fec_octets_for(last_block_);
    fec_octets_ = (blocks_ - 1) * (RS_N - RS_K) + last_fec_;
    if (fec_octets_ == 0)
        return false;

    buffer_.fill(0);
    state_ = State::Data;
    count_ = 0;
    bursts_++;
    return true;
}

void BurstDecoder::decode_data() {
    // Data octets go down the columns, one octet per block; the short last block drops out
    // once it is full. The FEC octets follow the same way in the last six columns.
    for (auto& row : blocks_table_)
        row.fill(0);
    size_t i = 0;
    for (size_t col = 0; col < RS_K && i < octets_; col++)
        for (size_t row = 0; row < blocks_ && i < octets_; row++)
            if (row != blocks_ - 1 || col < last_block_)
                blocks_table_[row][col] = buffer_[i++];
    for (size_t col = 0; col < RS_N - RS_K; col++)
        for (size_t row = 0; row < blocks_; row++)
            if (row != blocks_ - 1 || col < last_fec_)
                blocks_table_[row][RS_K + col] = buffer_[i++];

    // Rebuild the AVLC bit stream from the corrected blocks, in place of the received octets.
    size_t out = 0;
    for (size_t row = 0; row < blocks_; row++) {
        const bool last = row == blocks_ - 1;
        const size_t fec = last ? last_fec_ : RS_N - RS_K;
        if (fec) {
            uint8_t erasures[RS_N - RS_K];
            const size_t erased = RS_N - RS_K - fec;
            for (size_t e = 0; e < erased; e++)
                erasures[e] = RS_K + fec + e;
            if (rs_.decode(blocks_table_[row].data(), erasures, erased) < 0)
                return;
        }
        const size_t n = last ? last_block_ : RS_K;
        memcpy(&buffer_[out], blocks_table_[row].data(), n);
        out += n;
    }
    deframe(buffer_.data(), data_bits_);
}

// HDLC: frames between 0x7E flags, a 0 stuffed after five ones, bits least significant first.
void BurstDecoder::deframe(const uint8_t* data, const size_t bits) {
    size_t frame_bits = 0;
    uint8_t ones = 0;
    for (size_t i = 0; i < bits; i++) {
        const uint8_t b = (data[i >> 3] >> (i & 7)) & 1;
        if (b) {
            if (++ones > 6)
                return;  // abort sequence
        } else {
            if (ones == 5) {  // stuffed
                ones = 0;
                continue;
            }
            if (ones == 6) {  // flag: the frame ends before its 0111111
                const size_t len_bits = (frame_bits >= 7) ? frame_bits - 7 : 0;
                const size_t len = len_bits / 8;
                if (len_bits % 8 == 0 && len > 2 && crc_x25(frame_.data(), len) == 0xF0B8) {
                    frames_++;
                    handler_(context_, frame_.data(), len - 2);
                }
                frame_bits = 0;
                ones = 0;
                continue;
            }
            ones = 0;
        }
        if (frame_bits >= 8 * frame_.size())
            return;
        if ((frame_bits & 7) == 0)
            frame_[frame_bits >> 3] = 0;
        frame_[frame_bits >> 3] |= b << (frame_bits & 7);
        frame_bits++;
    }
}

// --- Demodulator --------------------------------------------------------------------------

std::complex<float> Demodulator::at(const uint32_t n, const float fraction) const {
    const auto a = iq_[n & (BUFFER - 1)];
    const auto b = iq_[(n + 1) & (BUFFER - 1)];
    return a + (b - a) * fraction;
}

// How well the 16 symbols ending at sample `end` match the training sequence: the phase errors,
// less their mean and a straight line (a carrier offset), squared and summed.
float Demodulator::preamble_metric(const uint32_t end, float& slope) const {
    float err[PREAMBLE];
    float prev = 0.0f, unwrap = 0.0f, mean = 0.0f;
    for (size_t k = 0; k < PREAMBLE; k++) {
        const uint32_t n = end - (uint32_t)((PREAMBLE - 1 - k) * SPS + 0.5f);
        const float e = wrap(phase_[n & (BUFFER - 1)] - PREAMBLE_PHASE[k] * PI_4);
        if (k) {
            // successive errors differ by the carrier offset per symbol plus noise
            unwrap += wrap(e - prev);
            err[k] = err[0] + unwrap;
        } else {
            err[k] = e;
        }
        prev = e;
        mean += err[k];
    }
    mean /= PREAMBLE;
    constexpr float x_mean = (PREAMBLE - 1) / 2.0f;
    float sxy = 0.0f, sxx = 0.0f;
    for (size_t k = 0; k < PREAMBLE; k++) {
        err[k] -= mean;
        sxy += (k - x_mean) * err[k];
        sxx += (k - x_mean) * (k - x_mean);
    }
    slope = sxy / sxx;
    float metric = 0.0f;
    for (size_t k = 0; k < PREAMBLE; k++) {
        const float r = err[k] - slope * (k - x_mean);
        metric += r * r;
    }
    return metric;
}

void Demodulator::execute(const std::complex<float> sample) {
    n_++;
    iq_[n_ & (BUFFER - 1)] = sample;
    phase_[n_ & (BUFFER - 1)] = fast_atan2(sample.imag(), sample.real());

    const float p = std::norm(sample);
    power_ += (p - power_) * 0.02f;
    // The noise floor follows the quiet level down quickly and up slowly.
    noise_ = (power_ < noise_) ? power_ : noise_ + (power_ - noise_) * 0.00002f;

    if (state_ == State::Symbols) {
        symbols();
        return;
    }

    // Only look for a preamble well above the noise.
    if (power_ < 4.0f * noise_ || n_ < BUFFER) {
        metric_[0] = metric_[1] = metric_[2] = 1e9f;
        return;
    }
    metric_[2] = metric_[1];
    metric_[1] = metric_[0];
    slope_[2] = slope_[1];
    slope_[1] = slope_[0];
    metric_[0] = preamble_metric(n_, slope_[0]);

    // At a minimum below the threshold: that sample ends the training sequence.
    if (metric_[1] < SYNC_THRESHOLD && metric_[1] <= metric_[0] && metric_[1] < metric_[2]) {
        // the minimum of a parabola through the three, between -0.5 and +0.5 sample
        const float d = metric_[2] - 2.0f * metric_[1] + metric_[0];
        float frac = (d > 0.0f) ? 0.5f * (metric_[2] - metric_[0]) / d : 0.0f;
        if (frac > 0.5f) frac = 0.5f;
        if (frac < -0.5f) frac = -0.5f;
        on_sync(n_ - 1, frac, slope_[1]);
    }
}

void Demodulator::on_sync(const uint32_t n, const float fraction, const float slope) {
    // fraction is relative to sample n; keep it in 0..1 from an integer sample
    uint32_t base = n;
    float f = fraction;
    if (f < 0.0f) {
        base--;
        f += 1.0f;
    }
    const auto c = at(base, f);
    prev_phase_ = fast_atan2(c.imag(), c.real());
    phase_step_ = slope;
    last_offset_hz_ = slope * 10500.0f / TWO_PI;
    syncs_++;

    next_frac_ = f + SPS;
    next_n_ = base + (uint32_t)next_frac_;
    next_frac_ -= (uint32_t)next_frac_;

    metric_[0] = metric_[1] = metric_[2] = 1e9f;
    burst_.start();
    state_ = State::Symbols;
    symbols();
}

void Demodulator::symbols() {
    // A symbol can be taken once the sample after it is in.
    while ((int32_t)(n_ - (next_n_ + 1)) >= 0) {
        const auto c = at(next_n_, next_frac_);
        const float ph = fast_atan2(c.imag(), c.real());
        float d = ph - prev_phase_ - phase_step_;
        while (d < 0.0f) d += TWO_PI;
        while (d >= TWO_PI) d -= TWO_PI;
        const uint8_t bits = GRAY[(int)(d / PI_4 + 0.5f) & 7];
        prev_phase_ = ph;

        next_frac_ += SPS;
        next_n_ += (uint32_t)next_frac_;
        next_frac_ -= (uint32_t)next_frac_;

        for (int i = 2; i >= 0; i--) {
            if (!burst_.bit((bits >> i) & 1)) {
                state_ = State::Search;
                return;
            }
        }
    }
}

}  // namespace vdl2
