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

/* MSC depuncturing (EN 300 401 section 11.3): L1 blocks of 128 mother bits with
 * PI(v+1), L2 with PI(v), then a 24-bit tail keeping 12. Derived, not tabled:
 * L1 + L2 = payload bits / 32 and L1*w(v+1) + L2*w(v) = (capacity - 12) / 4
 * solve uniquely, as adjacent vectors differ in weight by one. A schedule that
 * does not balance (corrupt FIG 0/1, unsupported profile) is rejected. */

#ifndef __DAB_MSC_H__
#define __DAB_MSC_H__

#include "dab_constants.hpp"
#include "dab_protection.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

struct MscSchedule {
    uint16_t l1{0};  // blocks punctured with pi1
    uint8_t pi1{0};  // puncturing vector index, 1..24
    uint16_t l2{0};  // blocks punctured with pi2
    uint8_t pi2{0};  // pi1 - 1
    uint32_t payload_bits{0};
    uint32_t mother_bits{0};
    uint32_t punctured_bits{0};  // equals capacity, size_cu * 64
    bool valid{false};
};

/* Whether proc_dab.cpp's staging (fft_/defer_stage_, finished up to two symbols
 * late) can carry it: at most two CIF symbols, the last by CIF symbol 17
 * (decode_cif()), the first from 2 (clear of the FIC's deferred finish). */
constexpr bool subchannel_placement_ok(uint16_t start_cu, uint16_t size_cu) {
    if (size_cu == 0)
        return false;
    const uint32_t first = static_cast<uint32_t>(start_cu) * 64u / 3072u;
    const uint32_t last = (static_cast<uint32_t>(start_cu + size_cu) * 64u - 1u) / 3072u;
    const uint32_t span = last - first + 1u;
    return span <= 2 && last + span <= 17 && first >= 2;
}

/* CIF symbol (index - 4) % 18 holds coded bits [3072 i, 3072 (i + 1)) of its
 * CIF; the subchannel is bits [64 start, 64 (start + size)). The masks below
 * are built from this. */
constexpr bool frame_symbol_carries(size_t index, uint16_t start_cu, uint16_t size_cu) {
    if (index <= kFicSymbols || index >= kSymbolsPerFrame)
        return false;
    const size_t bits = 2 * kCarriers;
    const size_t in_cif = (index - kFicSymbols - 1) % (kMscSymbols / kCifsPerFrame);
    const size_t first = in_cif * bits;
    const size_t sub_first = static_cast<size_t>(start_cu) * 64;
    const size_t sub_end = sub_first + static_cast<size_t>(size_cu) * 64;
    return first < sub_end && first + bits > sub_first;
}

/* One bit per frame symbol 0-95, built at configuration since the trellis
 * queries it every DMA buffer. */
struct FrameSymbolMask {
    uint32_t w[3]{};

    bool test(size_t s) const { return s < 96 && ((w[s >> 5] >> (s & 31)) & 1u) != 0; }
    void set(size_t s) { w[s >> 5] |= 1u << (s & 31); }
};

inline FrameSymbolMask subchannel_symbols(uint16_t start_cu, uint16_t size_cu) {
    FrameSymbolMask m{};
    for (size_t s = 0; s < kSymbolsPerFrame; ++s)
        if (frame_symbol_carries(s, start_cu, size_cu))
            m.set(s);
    return m;
}

/* Symbols the MSC trellis keeps out of: a transform runs when a symbol or the
 * next is wanted, so one either side of each wanted one, two after if deferred. */
inline FrameSymbolMask subchannel_window(const FrameSymbolMask& wanted, bool deferred) {
    FrameSymbolMask m{};
    for (size_t s = 0; s < 96; ++s) {
        if (wanted.test(s) || wanted.test(s + 1) || wanted.test(s + 2) || (s > 0 && wanted.test(s - 1)) ||
            (deferred && s > 1 && wanted.test(s - 2)))
            m.set(s);
    }
    return m;
}

/* valid == false if size and profile do not describe a real subchannel. */
MscSchedule msc_schedule(uint16_t size_cu, const ProtectionProfile& profile);

/* One CIF into the mother codeword, 0 (erasure) where a bit was removed.
 * Returns mother bits written. */
size_t depuncture_msc(const MscSchedule& schedule,
                      const int8_t* punctured,
                      int8_t* out,
                      size_t out_capacity);

/* Packed (dab_soft.hpp): `punctured` is a code run, `out` one word per trellis
 * step. Returns mother bits written. Groups are fetched as words: `punctured`
 * must be readable four bytes past its last code. */
size_t depuncture_msc_packed(const MscSchedule& schedule,
                             const uint8_t* punctured,
                             uint16_t* out,
                             size_t out_capacity_steps);

} /* namespace dab */

#endif /*__DAB_MSC_H__*/
