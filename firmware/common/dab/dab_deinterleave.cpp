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

#include "dab_deinterleave.hpp"

namespace dab {

uint8_t TimeDeinterleaver::branch_delay(uint8_t branch) {
    /* bitreverse of the 4-bit branch index. */
    const uint8_t b = static_cast<uint8_t>(branch & 0x0F);
    return static_cast<uint8_t>(((b & 1) << 3) | ((b & 2) << 1) | ((b & 4) >> 1) | ((b & 8) >> 3));
}

bool TimeDeinterleaver::configure(size_t bits_per_cif, uint8_t* storage, size_t storage_bytes) {
    if (bits_per_cif == 0 || (bits_per_cif % kInterleaverBranches) != 0)
        return false;
    if (bits_per_cif > kMaxCodedBitsPerCif)
        return false;
    /* A buffer smaller than storage_bytes_for() is refused, not overrun. */
    if (storage == nullptr || storage_bytes < storage_bytes_for(bits_per_cif))
        return false;

    storage_ = storage;
    storage_bytes_ = storage_bytes;

    bits_per_cif_ = bits_per_cif;
    per_branch_ = bits_per_cif / kInterleaverBranches;
    row_ = row_slots(bits_per_cif);

    size_t running = 0;
    for (uint8_t j = 0; j < kInterleaverBranches; ++j) {
        depth_[j] = static_cast<uint8_t>(kInterleaverLatencyCifs - branch_delay(j));
        offset_[j] = running;
        running += static_cast<size_t>(depth_[j]) * row_;
    }

    reset();
    return true;
}

void TimeDeinterleaver::reset() {
    pushed_ = 0;
    for (auto& h : head_)
        h = 0;
    if (storage_ != nullptr) {
        for (size_t i = 0; i < storage_bytes_; ++i)
            storage_[i] = 0;
    }
}

bool TimeDeinterleaver::end_cif() {
    for (uint8_t j = 0; j < kInterleaverBranches; ++j) {
        if (depth_[j] != 0)
            head_[j] = (head_[j] + 1) % depth_[j];
    }

    ++pushed_;
    return pushed_ > kInterleaverLatencyCifs;
}

namespace {

/* Three bytes, eight codes of three bits, slot k at bits 3k..3k+2. */
inline uint32_t load_group(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16);
}

inline void store_group(uint8_t* p, uint32_t g) {
    p[0] = static_cast<uint8_t>(g);
    p[1] = static_cast<uint8_t>(g >> 8);
    p[2] = static_cast<uint8_t>(g >> 16);
}

/* One group in two passes so the eight bytes are never live at once: incoming
 * codes into the new groups (a CIF byte holds slot k of branch 2p low, 2p+1
 * high), then stored codes out over them, both nibbles widened back to four
 * bits at once. Callers unroll so every shift is an immediate. */
__attribute__((always_inline)) inline void take_slot(const uint8_t* cell, uint32_t at, uint32_t& new0, uint32_t& new1) {
    const uint32_t in = *cell;
    new0 |= (in & 7u) << at;
    new1 |= ((in >> 4) & 7u) << at;
}

__attribute__((always_inline)) inline void give_slot(uint8_t* cell, uint32_t at, uint32_t old0, uint32_t old1) {
    uint32_t out = ((old0 >> at) & 7u) | (((old1 >> at) & 7u) << 4);
    out |= (out & 0x44u) << 1;
    *cell = static_cast<uint8_t>(out);
}

}  // namespace

bool TimeDeinterleaver::push_packed(uint8_t* cif) {
    if (bits_per_cif_ == 0 || storage_ == nullptr || cif == nullptr)
        return false;

    /* Value i is branch i % 16, slot i / 16, in byte i / 2, so branches 2p and
     * 2p+1 share byte p + 8 * slot (low and high nibble). A pair at a time
     * touches each byte once, and each history row advances a three-byte group
     * of eight slots. */
    for (uint8_t pair = 0; pair < kInterleaverBranches / 2; ++pair) {
        const uint8_t j0 = static_cast<uint8_t>(2 * pair);
        const uint8_t j1 = static_cast<uint8_t>(j0 + 1);
        const bool through = depth_[j1] == 0;

        uint8_t* row0 = &storage_[(offset_[j0] + head_[j0] * row_) / 8 * 3];
        uint8_t* row1 = through ? row0 : &storage_[(offset_[j1] + head_[j1] * row_) / 8 * 3];
        uint8_t* cell = cif + pair;

        for (size_t s = 0; s < per_branch_; s += 8, row0 += 3, row1 += 3) {
            const size_t n = (per_branch_ - s) < 8 ? (per_branch_ - s) : 8;
            const uint32_t old0 = load_group(row0);
            const uint32_t old1 = through ? 0 : load_group(row1);
            uint32_t new0 = 0;
            uint32_t new1 = 0;
            /* Branch 15 has no receiver delay, so its "history" is new1,
             * whose slots a half reads are complete once that half's takes
             * are done. Halves of four keep GCC from hoisting all eight loads
             * and spilling. */
            if (n == 8) {
                take_slot(cell + 0, 0, new0, new1);
                take_slot(cell + 8, 3, new0, new1);
                take_slot(cell + 16, 6, new0, new1);
                take_slot(cell + 24, 9, new0, new1);
                uint32_t h1 = through ? new1 : old1;
                give_slot(cell + 0, 0, old0, h1);
                give_slot(cell + 8, 3, old0, h1);
                give_slot(cell + 16, 6, old0, h1);
                give_slot(cell + 24, 9, old0, h1);
                take_slot(cell + 32, 12, new0, new1);
                take_slot(cell + 40, 15, new0, new1);
                take_slot(cell + 48, 18, new0, new1);
                take_slot(cell + 56, 21, new0, new1);
                h1 = through ? new1 : old1;
                give_slot(cell + 32, 12, old0, h1);
                give_slot(cell + 40, 15, old0, h1);
                give_slot(cell + 48, 18, old0, h1);
                give_slot(cell + 56, 21, old0, h1);
            } else {
                for (size_t k = 0; k < n; ++k) {
                    const uint32_t at = 3 * static_cast<uint32_t>(k);
                    take_slot(cell + 8 * k, at, new0, new1);
                    give_slot(cell + 8 * k, at, old0, through ? new1 : old1);
                }
            }
            cell += 8 * n;
            store_group(row0, new0);
            if (!through)
                store_group(row1, new1);
        }
    }

    return end_cif();
}

} /* namespace dab */
