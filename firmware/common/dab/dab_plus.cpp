/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "dab_plus.hpp"
#include <cstring>

namespace dab {

/* ---- Reed-Solomon ---- */

namespace {
constexpr uint16_t kFieldPoly = 0x11D;
}

ReedSolomon::ReedSolomon() {
    uint16_t x = 1;
    for (int i = 0; i < 255; ++i) {
        exp_[i] = static_cast<uint8_t>(x);
        log_[x] = static_cast<uint8_t>(i);
        x <<= 1;
        if (x & 0x100)
            x ^= kFieldPoly;
    }
    for (int i = 255; i < 512; ++i)
        exp_[i] = exp_[i - 255];
    log_[0] = 0;

    std::memset(generator_, 0, sizeof(generator_));
    generator_[0] = 1;
    size_t degree = 0;

    for (size_t i = 0; i < kRsParity; ++i) {
        const uint8_t root = exp_[i];
        ++degree;
        for (size_t j = degree; j > 0; --j)
            generator_[j] = static_cast<uint8_t>(generator_[j - 1] ^ mul(generator_[j], root));
        generator_[0] = mul(generator_[0], root);
    }

    for (size_t i = 0; i < kRsParity; ++i)
        for (size_t v = 0; v < 256; ++v)
            root_mul_[i][v] = mul(static_cast<uint8_t>(v), exp_[i]);
}

void ReedSolomon::encode(uint8_t block[kRsN]) {
    uint8_t parity[kRsParity]{};

    for (size_t i = 0; i < kRsK; ++i) {
        const uint8_t feedback = static_cast<uint8_t>(block[i] ^ parity[0]);

        for (size_t j = 0; j < kRsParity - 1; ++j)
            parity[j] = static_cast<uint8_t>(parity[j + 1] ^ mul(feedback, generator_[kRsParity - 1 - j]));

        parity[kRsParity - 1] = mul(feedback, generator_[0]);
    }

    for (size_t i = 0; i < kRsParity; ++i)
        block[kRsK + i] = parity[i];
}

int ReedSolomon::decode(uint8_t block[kRsN]) const {
    /* S_i = r(a^i), block[0] the highest-degree coefficient. */
    uint8_t syndrome[kRsParity];
    bool all_zero = true;

    /* Horner, kRsN loads rather than kRsParity * kRsN: every block pays this,
     * so it is decode()'s hot part. Two passes of five register accumulators,
     * not ten stored every step. */
    static_assert(kRsParity % 5 == 0, "syndromes are taken five per pass");
    uint8_t acc[kRsParity];
    for (size_t i = 0; i < kRsParity; i += 5) {
        const uint8_t* const t0 = root_mul_[i];
        const uint8_t* const t1 = root_mul_[i + 1];
        const uint8_t* const t2 = root_mul_[i + 2];
        const uint8_t* const t3 = root_mul_[i + 3];
        const uint8_t* const t4 = root_mul_[i + 4];
        uint32_t a0 = 0, a1 = 0, a2 = 0, a3 = 0, a4 = 0;
        for (size_t k = 0; k < kRsN; ++k) {
            const uint32_t b = block[k];
            a0 = t0[a0] ^ b;
            a1 = t1[a1] ^ b;
            a2 = t2[a2] ^ b;
            a3 = t3[a3] ^ b;
            a4 = t4[a4] ^ b;
        }
        acc[i] = static_cast<uint8_t>(a0);
        acc[i + 1] = static_cast<uint8_t>(a1);
        acc[i + 2] = static_cast<uint8_t>(a2);
        acc[i + 3] = static_cast<uint8_t>(a3);
        acc[i + 4] = static_cast<uint8_t>(a4);
    }
    for (size_t i = 0; i < kRsParity; ++i) {
        syndrome[i] = acc[i];
        if (acc[i] != 0)
            all_zero = false;
    }

    if (all_zero)
        return 0;

    /* Berlekamp-Massey. */
    uint8_t lambda[kRsParity + 1]{};
    uint8_t previous[kRsParity + 1]{};
    uint8_t temp[kRsParity + 1]{};
    lambda[0] = 1;
    previous[0] = 1;

    size_t l = 0;
    size_t m = 1;
    uint8_t b = 1;

    for (size_t n = 0; n < kRsParity; ++n) {
        uint8_t delta = syndrome[n];
        for (size_t i = 1; i <= l; ++i)
            delta ^= mul(lambda[i], syndrome[n - i]);

        if (delta == 0) {
            ++m;
        } else if (2 * l <= n) {
            std::memcpy(temp, lambda, sizeof(lambda));
            const uint8_t scale = div(delta, b);
            for (size_t i = 0; i + m <= kRsParity; ++i)
                lambda[i + m] ^= mul(scale, previous[i]);
            l = n + 1 - l;
            std::memcpy(previous, temp, sizeof(temp));
            b = delta;
            m = 1;
        } else {
            const uint8_t scale = div(delta, b);
            for (size_t i = 0; i + m <= kRsParity; ++i)
                lambda[i + m] ^= mul(scale, previous[i]);
            ++m;
        }
    }

    if (l > kRsT)
        return -1;

    /* Chien search: position k holds x^(kRsN-1-k), so a root at a^-e marks an
     * error at exponent e. */
    size_t positions[kRsT];
    uint8_t x_values[kRsT];
    size_t found = 0;

    for (size_t k = 0; k < kRsN; ++k) {
        const int exponent = static_cast<int>(kRsN - 1 - k);
        const uint8_t x_inv = pow_alpha(-exponent);

        uint8_t sum = 0;
        uint8_t power = 1;
        for (size_t i = 0; i <= l; ++i) {
            sum ^= mul(lambda[i], power);
            power = mul(power, x_inv);
        }

        if (sum == 0) {
            if (found >= kRsT)
                return -1;
            positions[found] = k;
            x_values[found] = pow_alpha(exponent);
            ++found;
        }
    }

    if (found != l)
        return -1;

    /* Omega(x) = S(x) * Lambda(x) mod x^(2t). */
    uint8_t omega[kRsParity]{};
    for (size_t i = 0; i < kRsParity; ++i) {
        uint8_t sum = 0;
        for (size_t j = 0; j <= i && j <= l; ++j)
            sum ^= mul(syndrome[i - j], lambda[j]);
        omega[i] = sum;
    }

    /* Forney, with the first generator root at a^0. */
    for (size_t idx = 0; idx < found; ++idx) {
        const uint8_t x = x_values[idx];
        const uint8_t x_inv = div(1, x);

        uint8_t omega_val = 0;
        uint8_t power = 1;
        for (size_t i = 0; i < kRsParity; ++i) {
            omega_val ^= mul(omega[i], power);
            power = mul(power, x_inv);
        }

        /* Formal derivative of Lambda: only odd-degree terms survive. */
        uint8_t deriv = 0;
        power = 1;
        for (size_t i = 1; i <= l; i += 2) {
            deriv ^= mul(lambda[i], power);
            power = mul(power, mul(x_inv, x_inv));
        }

        if (deriv == 0)
            return -1;

        const uint8_t magnitude = mul(x, div(omega_val, deriv));
        block[positions[idx]] ^= magnitude;
    }

    for (size_t i = 0; i < kRsParity; ++i) {
        uint8_t s = 0;
        const uint8_t root = exp_[i];
        for (size_t k = 0; k < kRsN; ++k)
            s = static_cast<uint8_t>(mul(s, root) ^ block[k]);
        if (s != 0)
            return -1;
    }

    return static_cast<int>(found);
}

/* ---- DAB+ superframe ---- */

uint8_t audio_units_for(uint8_t dac_rate, uint8_t sbr_flag) {
    if (sbr_flag)
        return dac_rate ? 3 : 2;
    return dac_rate ? 6 : 4;
}

bool audio_unit_crc_ok(const uint8_t* unit, size_t length) {
    if (unit == nullptr || length < 3)
        return false;

    const uint16_t computed = static_cast<uint16_t>(crc16_ccitt(unit, length - 2) ^ 0xFFFF);
    const uint16_t stored = static_cast<uint16_t>((unit[length - 2] << 8) | unit[length - 1]);
    return computed == stored;
}

void audio_unit_crc_stamp(uint8_t* unit, size_t length) {
    if (unit == nullptr || length < 3)
        return;

    const uint16_t computed = static_cast<uint16_t>(crc16_ccitt(unit, length - 2) ^ 0xFFFF);
    unit[length - 2] = static_cast<uint8_t>(computed >> 8);
    unit[length - 1] = static_cast<uint8_t>(computed & 0xFF);
}

static uint8_t g_rs_last_good = 0;
static uint8_t g_rs_last_bad = 0;
static uint8_t g_rs_last_total = 0;

uint8_t rs_last_good_words() {
    return g_rs_last_good;
}
uint8_t rs_last_bad_words() {
    return g_rs_last_bad;
}
uint8_t rs_last_total_words() {
    return g_rs_last_total;
}

static uint8_t g_rs_zero_why = 0;
uint8_t rs_zero_why() {
    return g_rs_zero_why;
}

size_t rs_correct_superframe(const uint8_t* raw, size_t superframe_bytes, uint8_t* out, size_t out_capacity, bool bail) {
    if (raw == nullptr || out == nullptr) {
        g_rs_zero_why = 1;
        return 0;
    }
    if (superframe_bytes == 0 || (superframe_bytes % kRsBlockLength) != 0) {
        g_rs_zero_why = 2;
        return 0;
    }

    const size_t n = superframe_bytes / kRsBlockLength;
    const size_t corrected = n * kRsDataLength;
    if (out_capacity < corrected) {
        g_rs_zero_why = 3;
        return 0;
    }
    g_rs_zero_why = 0;

    /* Static: decode() writes no member, so one instance serves every call (one
     * M4 thread), and the tables stay off the small stack. */
    static const ReedSolomon rs;
    uint8_t word[kRsBlockLength];
    uint8_t good = 0;
    uint8_t bad = 0;

    /* Give up after kRsBailRun consecutive bad words. A wrong boundary fails
     * nearly every word, a right one on a marginal channel only some; bailing
     * on the first costs sensitivity, and 3 beat never. */
    constexpr uint8_t kRsBailRun = 3;
    uint8_t run_bad = 0;

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < kRsBlockLength; j++)
            word[j] = raw[i + j * n];

        if (rs.decode(word) < 0) {
            /* Not a failed superframe: the firecode and unit CRCs decide. */
            if (bad < 0xFF)
                ++bad;
            if (run_bad < 0xFF)
                ++run_bad;
            if (bail && run_bad >= kRsBailRun) {
                g_rs_last_bad = bad;
                g_rs_last_total = static_cast<uint8_t>(n > 0xFF ? 0xFF : n);
                return 0;
            }
            for (size_t j = 0; j < kRsDataLength; j++)
                out[i + j * n] = raw[i + j * n];
            continue;
        }
        run_bad = 0;
        if (good < 0xFF)
            ++good;

        for (size_t j = 0; j < kRsDataLength; j++)
            out[i + j * n] = word[j];
    }

    g_rs_last_good = good;
    g_rs_last_bad = bad;
    g_rs_last_total = static_cast<uint8_t>(n > 0xFF ? 0xFF : n);

    return corrected;
}

bool SuperframeAssembler::configure(size_t logical_frame_bytes) {
    if (logical_frame_bytes == 0 || logical_frame_bytes > kMaxLogicalFrameBytes)
        return false;

    const size_t superframe_bytes = logical_frame_bytes * kSuperframeLogicalFrames;
    if ((superframe_bytes % kRsBlockLength) != 0)
        return false;

    if ((superframe_bytes / kRsBlockLength) * kRsDataLength > kMaxCorrectedBytes)
        return false;

    logical_frame_bytes_ = logical_frame_bytes;
    reset();
    return true;
}

void SuperframeAssembler::reset() {
    filled_ = 0;
    synchronised_ = false;
    miss_tests_ = 0xFF;
    unit_count_ = 0;
    header_ = SuperframeHeader{};
}

bool SuperframeAssembler::push_logical_frame(const uint8_t* data) {
    if (logical_frame_bytes_ == 0 || data == nullptr)
        return false;

    const size_t superframe_bytes = logical_frame_bytes_ * kSuperframeLogicalFrames;

    std::memcpy(&buffer_[filled_], data, logical_frame_bytes_);
    filled_ += logical_frame_bytes_;

    if (filled_ < superframe_bytes)
        return false;

    /* Superframes after a loss for which the old phase's boundary still gets
     * full Reed-Solomon instead of the bail. */
    constexpr size_t kSfPhaseMemory = 4;
    const bool at_phase = synchronised_ || (miss_tests_ % kSuperframeLogicalFrames == 0 &&
                                            miss_tests_ <= kSuperframeLogicalFrames * kSfPhaseMemory);
    if (try_parse(at_phase)) {
        filled_ = 0;
        synchronised_ = true;
        miss_tests_ = 0;
        return true;
    }
    if (synchronised_)
        miss_tests_ = 0;
    if (miss_tests_ < 0xFF)
        ++miss_tests_;

    synchronised_ = false;
    std::memmove(&buffer_[0], &buffer_[logical_frame_bytes_], superframe_bytes - logical_frame_bytes_);
    filled_ = superframe_bytes - logical_frame_bytes_;
    return false;
}

bool SuperframeAssembler::try_parse(bool full) {
    if (parse_calls_ < 0xFFFF)
        ++parse_calls_;
    const size_t raw_bytes = logical_frame_bytes_ * kSuperframeLogicalFrames;

    const size_t superframe_bytes = rs_correct_superframe(buffer_, raw_bytes, corrected_, sizeof(corrected_), !full);
    if (superframe_bytes == 0) {
        if (rs_zero_ < 0xFFFF) ++rs_zero_;
        return false;
    }
    if (rs_last_bad_words() > 0 && rs_fail_ < 0xFFFF)
        ++rs_fail_;

    if (superframe_bytes < 11) {
        if (rs_zero_ < 0xFFFF) ++rs_zero_;
        return false;
    }
    if (!firecode_ok(corrected_)) {
        if (firecode_fail_ < 0xFFFF)
            ++firecode_fail_;
        return false;
    }

    const uint8_t config = corrected_[2];
    SuperframeHeader header{};
    header.dac_rate = static_cast<uint8_t>((config >> 6) & 1);
    header.sbr_flag = static_cast<uint8_t>((config >> 5) & 1);
    header.aac_channel_mode = static_cast<uint8_t>((config >> 4) & 1);
    header.ps_flag = static_cast<uint8_t>((config >> 3) & 1);
    header.mpeg_surround = static_cast<uint8_t>(config & 0x07);
    header.audio_unit_count = audio_units_for(header.dac_rate, header.sbr_flag);

    const size_t count = header.audio_unit_count;
    if (count == 0 || count > kMaxAudioUnits) {
        if (header_fail_ < 0xFFFF) ++header_fail_;
        return false;
    }

    const size_t address_bits = (count - 1) * 12;
    const size_t address_bytes = (address_bits + 7) / 8;
    const size_t header_bytes = 3 + address_bytes;
    if (header_bytes > superframe_bytes) {
        if (header_fail_ < 0xFFFF) ++header_fail_;
        return false;
    }

    size_t starts[kMaxAudioUnits]{};
    starts[0] = header_bytes;

    for (size_t i = 1; i < count; ++i) {
        const size_t bit_offset = (i - 1) * 12;
        const size_t byte_index = 3 + (bit_offset / 8);
        uint16_t value;
        if ((bit_offset % 8) == 0)
            value = static_cast<uint16_t>((corrected_[byte_index] << 4) | (corrected_[byte_index + 1] >> 4));
        else
            value = static_cast<uint16_t>(((corrected_[byte_index] & 0x0F) << 8) | corrected_[byte_index + 1]);
        starts[i] = value;
    }

    for (size_t i = 1; i < count; ++i) {
        if (starts[i] <= starts[i - 1] || starts[i] >= superframe_bytes) {
            if (header_fail_ < 0xFFFF) ++header_fail_;
            return false;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        const size_t end = (i + 1 < count) ? starts[i + 1] : superframe_bytes;
        if (end <= starts[i]) {
            if (header_fail_ < 0xFFFF) ++header_fail_;
            return false;
        }

        units_[i].data = &corrected_[starts[i]];
        units_[i].length = end - starts[i];
        units_[i].crc_ok = audio_unit_crc_ok(units_[i].data, units_[i].length);
    }

    header_ = header;
    unit_count_ = count;
    return true;
}

} /* namespace dab */
