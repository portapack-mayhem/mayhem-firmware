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

#include "dab_reed_solomon.hpp"

#include <cstring>

namespace dab {

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

} /* namespace dab */
