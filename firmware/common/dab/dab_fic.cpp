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

#include "dab_fic.hpp"
#include "dab_protection.hpp"
#include "dab_soft.hpp"

namespace dab {

namespace {

constexpr size_t kBlocksPi16 = 21;
constexpr size_t kBlocksPi15 = 3;

/* 32 payload bits: four applications of a 32-bit puncturing vector. */
constexpr size_t kMotherBitsPerBlock = 128;

constexpr size_t kTailMotherBits = 24;

size_t expand(const int8_t* punctured, size_t& taken, uint32_t vector, size_t mother_bits, int8_t* out, size_t written, size_t capacity) {
    for (size_t i = 0; i < mother_bits && written < capacity; ++i) {
        const bool kept = ((vector >> (31 - i % 32)) & 1) != 0;
        out[written++] = kept ? punctured[taken++] : 0;
    }
    return written;
}

/* Kept bits per group of four, MSB first (table 29): PI16 is 1110 throughout,
 * PI15 ends in 1100, the tail is 1100. softcheck holds this to depuncture_fic(). */
constexpr uint8_t kKeepPi16[8] = {3, 3, 3, 3, 3, 3, 3, 3};
constexpr uint8_t kKeepPi15[8] = {3, 3, 3, 3, 3, 3, 3, 2};
constexpr uint8_t kKeepTail[8] = {2, 2, 2, 2, 2, 2, 2, 2};

/* Each step: four codes from the next four int8 at once, masked to the kept
 * ones. Reads up to three values past the last kept. */
const int8_t* put_fic_steps(const int8_t* __restrict src, uint16_t* __restrict dst, size_t steps, const uint8_t* keep) {
    static constexpr uint16_t kMask[5] = {0x0000u, 0x000Fu, 0x00FFu, 0x0FFFu, 0xFFFFu};
    for (size_t i = 0; i < steps; ++i) {
        uint32_t w;
        __builtin_memcpy(&w, src, sizeof(w));
        const uint32_t k = keep[i & 7];
        dst[i] = static_cast<uint16_t>(soft_codes4(w) & kMask[k]);
        src += k;
    }
    return src;
}

}  // namespace

size_t depuncture_fic_packed(const int8_t* punctured, uint16_t* out, size_t out_capacity_steps) {
    if (punctured == nullptr || out == nullptr || out_capacity_steps * 4 < kFicMotherBits)
        return 0;

    /* `punctured` must stay readable three bytes past its end. */
    constexpr size_t kStepsPerBlock = kMotherBitsPerBlock / 4;
    const int8_t* src = punctured;
    src = put_fic_steps(src, out, kBlocksPi16 * kStepsPerBlock, kKeepPi16);
    src = put_fic_steps(src, out + kBlocksPi16 * kStepsPerBlock, kBlocksPi15 * kStepsPerBlock, kKeepPi15);
    put_fic_steps(src, out + (kBlocksPi16 + kBlocksPi15) * kStepsPerBlock, kTailMotherBits / 4, kKeepTail);
    return kFicMotherBits;
}

size_t depuncture_fic(const int8_t* punctured, int8_t* out, size_t out_capacity) {
    if (punctured == nullptr || out == nullptr || out_capacity < kFicMotherBits)
        return 0;

    const uint32_t pi16 = puncturing_vector(16);
    const uint32_t pi15 = puncturing_vector(15);
    if (pi16 == 0 || pi15 == 0)
        return 0;

    size_t taken = 0;
    size_t written = 0;

    for (size_t b = 0; b < kBlocksPi16; ++b)
        written = expand(punctured, taken, pi16, kMotherBitsPerBlock, out, written, out_capacity);

    for (size_t b = 0; b < kBlocksPi15; ++b)
        written = expand(punctured, taken, pi15, kMotherBitsPerBlock, out, written, out_capacity);

    /* Tail: 1100 over 24 mother bits. */
    written = expand(punctured, taken, 0xCCCCCCCCu, kTailMotherBits, out, written, out_capacity);

    return written;
}

} /* namespace dab */
