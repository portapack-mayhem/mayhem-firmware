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

#include "dab_phaseref.hpp"

namespace dab {

namespace {

/* EN 300 401 table 44, mode I: 48 blocks of 32 carriers,
 * phi_k = pi/2 * (h[i][k - k_min] + n). As printed, so a transcription error
 * can be caught by reading; k_max = k_min + 31 is not stored. */
struct PhaseRow {
    int16_t k_min;
    int8_t i;
    int8_t n;
};

constexpr PhaseRow kTable44[48] = {
    {-768, 0, 1},
    {-736, 1, 2},
    {-704, 2, 0},
    {-672, 3, 1},
    {-640, 0, 3},
    {-608, 1, 2},
    {-576, 2, 2},
    {-544, 3, 3},
    {-512, 0, 2},
    {-480, 1, 1},
    {-448, 2, 2},
    {-416, 3, 3},
    {-384, 0, 1},
    {-352, 1, 2},
    {-320, 2, 3},
    {-288, 3, 3},
    {-256, 0, 2},
    {-224, 1, 2},
    {-192, 2, 2},
    {-160, 3, 1},
    {-128, 0, 1},
    {-96, 1, 3},
    {-64, 2, 1},
    {-32, 3, 2},
    {1, 0, 3},
    {33, 3, 1},
    {65, 2, 1},
    {97, 1, 1},
    {129, 0, 2},
    {161, 3, 2},
    {193, 2, 1},
    {225, 1, 0},
    {257, 0, 2},
    {289, 3, 2},
    {321, 2, 3},
    {353, 1, 3},
    {385, 0, 0},
    {417, 3, 2},
    {449, 2, 1},
    {481, 1, 3},
    {513, 0, 3},
    {545, 3, 3},
    {577, 2, 3},
    {609, 1, 0},
    {641, 0, 3},
    {673, 3, 0},
    {705, 2, 1},
    {737, 1, 1},
};

/* EN 300 401 table 43, unfolded as printed. */
constexpr int8_t kH[4][32] = {
    {0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1,
     0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1},
    {0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0,
     0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0},
    {0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3,
     0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3},
    {0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2,
     0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2},
};

constexpr size_t kRows = 48;
constexpr size_t kBlockCarriers = 32;
constexpr size_t kPackedBytes = kCarriers / 4;

/* The bins the standard leaves empty: DC, and everything above the top
 * carrier up to the bottom one, which wraps to the negative half. */
constexpr size_t kFirstEmptyBin = kCarriers / 2 + 1;
constexpr size_t kLastEmptyBin = kFftSize - kCarriers / 2 - 1;

struct PackedPhases {
    uint8_t byte[kPackedBytes];
};

/* An overlap would OR two phases in build_packed(); a gap would leave a
 * silent zero. */
constexpr size_t table_coverage() {
    bool seen[kCarriers] = {};
    size_t n = 0;
    for (size_t r = 0; r < kRows; ++r) {
        for (size_t j = 0; j < kBlockCarriers; ++j) {
            const int k = kTable44[r].k_min + static_cast<int>(j);
            if (k == 0 || k < -768 || k > 768)
                return 0;
            const size_t pos = carrier_position(k);
            if (seen[pos])
                return 0;
            seen[pos] = true;
            ++n;
        }
    }
    return n;
}

static_assert(table_coverage() == kCarriers,
              "table 44 must tile all 1536 carriers exactly once");

/* Compile time, so only the 384-byte result reaches the image. */
constexpr PackedPhases build_packed() {
    PackedPhases p{};
    for (size_t r = 0; r < kRows; ++r) {
        for (size_t j = 0; j < kBlockCarriers; ++j) {
            const int k = kTable44[r].k_min + static_cast<int>(j);
            const size_t pos = carrier_position(k);
            const uint8_t v = static_cast<uint8_t>((kH[kTable44[r].i][j] + kTable44[r].n) & 3);
            p.byte[pos >> 2] |= static_cast<uint8_t>(v << (2 * (pos & 3)));
        }
    }
    return p;
}

constexpr PackedPhases kPacked = build_packed();

inline uint8_t phase_at(size_t index) {
    return static_cast<uint8_t>((kPacked.byte[index >> 2] >> (2 * (index & 3))) & 3);
}

}  // namespace

uint8_t prs_phase(size_t index) {
    return index < kCarriers ? phase_at(index) : 0;
}

uint8_t prs_phase_of_carrier(int16_t k) {
    if (k == 0 || k < -768 || k > 768)
        return 0xFF;
    return phase_at(carrier_position(k));
}

void prs_conjugate_multiply(FftComplex* bins, size_t bin_shift) {
    if (bins == nullptr)
        return;

    /* conj(exp(j*pi/2*n)) is 1, -j, -1, +j: a swap and a sign, no multiply. */
    for (size_t p = 0; p < kCarriers; ++p) {
        const int16_t k = prs_carrier_at(p);
        const size_t bin = (static_cast<size_t>(k + static_cast<int>(kFftSize)) + bin_shift) & (kFftSize - 1);
        FftComplex& c = bins[bin];
        const int16_t re = c.re;
        const int16_t im = c.im;
        switch (phase_at(p)) {
            case 0:
                break;
            case 1:
                c.re = im;
                c.im = static_cast<int16_t>(-static_cast<int>(re));
                break;
            case 2:
                c.re = static_cast<int16_t>(-static_cast<int>(re));
                c.im = static_cast<int16_t>(-static_cast<int>(im));
                break;
            default:
                c.re = static_cast<int16_t>(-static_cast<int>(im));
                c.im = re;
                break;
        }
    }

    /* Empty bins would add a flat pedestal to the impulse response's peak
     * search. */
    bins[bin_shift & (kFftSize - 1)] = FftComplex{0, 0};
    for (size_t b = kFirstEmptyBin; b <= kLastEmptyBin; ++b)
        bins[(b + bin_shift) & (kFftSize - 1)] = FftComplex{0, 0};
}

} /* namespace dab */
