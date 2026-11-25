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

#include "dab_msc.hpp"
#include "dab_constants.hpp"
#include "dab_soft.hpp"

namespace dab {

namespace {

constexpr size_t kMotherBitsPerBlock = 128;
constexpr size_t kTailMotherBits = 24;
constexpr size_t kTailKeptBits = 12;

}  // namespace

MscSchedule msc_schedule(uint16_t size_cu, const ProtectionProfile& profile) {
    MscSchedule schedule{};

    if (profile.kind != ProtectionKind::Eep)
        return schedule;

    const uint32_t bitrate = eep_bitrate(size_cu, profile);
    if (bitrate == 0)
        return schedule;

    const uint32_t payload_bits = (bitrate * kFrameDurationMs) / (1000 * kCifsPerFrame);
    if (payload_bits == 0 || (payload_bits % 32) != 0)
        return schedule;

    const uint32_t capacity = static_cast<uint32_t>(size_cu) * 64;
    if (capacity <= kTailKeptBits)
        return schedule;

    const uint32_t blocks = payload_bits / 32;
    const uint32_t kept_quarters = (capacity - kTailKeptBits) / 4;
    if (((capacity - kTailKeptBits) % 4) != 0)
        return schedule;

    /* L1*(w+1) + L2*w = kept_quarters with L1 + L2 = blocks gives
     * w = floor(kept_quarters / blocks) and L1 = kept_quarters - blocks*w. */
    const uint32_t w2 = kept_quarters / blocks;
    const uint32_t l1 = kept_quarters - blocks * w2;
    const uint32_t l2 = blocks - l1;

    /* Puncturing vector i keeps i + 8 bits, so weight w maps to index w - 8. */
    if (w2 < 9 || w2 > 32)
        return schedule;

    const uint32_t pi2 = w2 - 8;
    const uint32_t pi1 = pi2 + 1;
    if (pi1 > 24)
        return schedule;

    schedule.l1 = static_cast<uint16_t>(l1);
    schedule.l2 = static_cast<uint16_t>(l2);
    schedule.pi1 = static_cast<uint8_t>(pi1);
    schedule.pi2 = static_cast<uint8_t>(pi2);
    schedule.payload_bits = payload_bits;
    schedule.mother_bits = 4 * (payload_bits + kConstraintLength - 1);
    schedule.punctured_bits = capacity;

    const uint32_t reconstructed =
        4 * (l1 * puncturing_weight(schedule.pi1) + l2 * puncturing_weight(schedule.pi2)) + kTailKeptBits;
    if (reconstructed != capacity)
        return schedule;

    if (4 * blocks * 32 + kTailMotherBits != schedule.mother_bits)
        return schedule;

    schedule.valid = true;
    return schedule;
}

namespace {

/* Per 4-bit group of a vector. Every group in table 29 keeps a prefix (1000,
 * 1100, 1110, 1111), so it expands to the next `keep` soft values and 4 - keep
 * erasures: one unaligned load, a mask, a store. build() fails otherwise. */
struct GroupPlan {
    uint32_t mask[8];
    uint8_t keep[8];

    bool build(uint32_t vector) {
        /* Byte 0 of a little-endian word is the group's first, kept, bit. */
        static constexpr uint32_t kPrefix[5] = {0u, 0x000000FFu, 0x0000FFFFu, 0x00FFFFFFu, 0xFFFFFFFFu};
        for (size_t g = 0; g < 8; ++g) {
            const uint32_t nib = (vector >> (28 - 4 * g)) & 0xFu;
            uint32_t k = 0;
            while (k < 4 && ((nib >> (3 - k)) & 1) != 0)
                ++k;
            if (k == 0 || (nib & (0xFu >> k)) != 0)
                return false; /* not a prefix -- fall back */
            mask[g] = kPrefix[k];
            keep[g] = static_cast<uint8_t>(k);
        }
        return true;
    }
};

/* Per-bit fallback and tail. By value: a capturing lambda's captures are
 * stack pointers at -Os, two dependent loads per access. */
struct BitCursor {
    size_t taken;
    size_t written;
};

BitCursor expand_bits(uint32_t vector,
                      size_t mother_bits,
                      const int8_t* punctured,
                      int8_t* out,
                      size_t out_capacity,
                      BitCursor cursor) {
    size_t taken = cursor.taken;
    size_t written = cursor.written;
    for (size_t i = 0; i < mother_bits && written < out_capacity; ++i) {
        const size_t bit = i & 31u;
        const bool kept = ((vector >> (31 - bit)) & 1) != 0;
        out[written++] = kept ? punctured[taken++] : 0;
    }
    return {taken, written};
}

/* A kept group is the next `keep` codes; the other nibbles stay 0, erasures. */
struct StepPlan {
    uint16_t mask[8];
    uint8_t keep[8];

    explicit StepPlan(const GroupPlan& g) {
        for (size_t i = 0; i < 8; ++i) {
            keep[i] = g.keep[i];
            mask[i] = static_cast<uint16_t>((1u << (4u * g.keep[i])) - 1u);
        }
    }
};

/* Cursor by value so it stays in registers; through a reference GCC stores it
 * after every step. */
__attribute__((noinline)) size_t put_block(const uint8_t* __restrict run, size_t code, uint16_t* __restrict dst, const StepPlan& __restrict p) {
    for (size_t g = 0; g < 8; ++g) {
        uint32_t w;
        __builtin_memcpy(&w, run + (code >> 1), sizeof(w));
        w >>= (code & 1u) << 2;
        dst[g] = static_cast<uint16_t>(w & p.mask[g]);
        code += p.keep[g];
    }
    return code;
}

}  // namespace

size_t depuncture_msc_packed(const MscSchedule& schedule,
                             const uint8_t* punctured,
                             uint16_t* out,
                             size_t out_capacity_steps) {
    static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "groups are fetched as little-endian words");
    if (!schedule.valid || punctured == nullptr || out == nullptr)
        return 0;
    if (out_capacity_steps * 4 < schedule.mother_bits)
        return 0;

    const uint32_t v1 = puncturing_vector(schedule.pi1);
    const uint32_t v2 = puncturing_vector(schedule.pi2);
    if (v1 == 0 || v2 == 0)
        return 0;

    /* No per-bit path here: a vector without prefix groups is refused. */
    GroupPlan p1{};
    GroupPlan p2{};
    if (!p1.build(v1) || !p2.build(v2))
        return 0;

    size_t code = 0;
    uint16_t* dst = out;
    const StepPlan s1{p1};
    const StepPlan s2{p2};
    const uint32_t reps1 = static_cast<uint32_t>(schedule.l1) * (kMotherBitsPerBlock / 32);
    const uint32_t reps2 = static_cast<uint32_t>(schedule.l2) * (kMotherBitsPerBlock / 32);
    for (uint32_t r = 0; r < reps1; ++r, dst += 8)
        code = put_block(punctured, code, dst, s1);
    for (uint32_t r = 0; r < reps2; ++r, dst += 8)
        code = put_block(punctured, code, dst, s2);

    /* Tail: 1100 over 24 mother bits. */
    for (size_t i = 0; i < kTailMotherBits / 4; ++i, code += 2) {
        uint32_t w;
        __builtin_memcpy(&w, punctured + (code >> 1), sizeof(w));
        *dst++ = static_cast<uint16_t>((w >> ((code & 1u) << 2)) & 0xFFu);
    }
    return static_cast<size_t>(dst - out) * 4;
}

size_t depuncture_msc(const MscSchedule& schedule,
                      const int8_t* punctured,
                      int8_t* out,
                      size_t out_capacity) {
    if (!schedule.valid || punctured == nullptr || out == nullptr)
        return 0;
    if (out_capacity < schedule.mother_bits)
        return 0;

    const uint32_t v1 = puncturing_vector(schedule.pi1);
    const uint32_t v2 = puncturing_vector(schedule.pi2);
    if (v1 == 0 || v2 == 0)
        return 0;

    GroupPlan p1{};
    GroupPlan p2{};
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    const bool fast = p1.build(v1) && p2.build(v2);
#else
    /* The word form assumes little-endian; the per-bit path does not. */
    (void)p1;
    (void)p2;
    const bool fast = false;
#endif

    BitCursor cursor{0, 0};

    if (fast) {
        /* The tail stays per-bit so the word load cannot overrun: the blocks
         * take exactly capacity - 12 soft values (msc_schedule()), so the last
         * word read ends at least ten bytes inside the buffer. */
        const int8_t* src = punctured;
        int8_t* dst = out;

        const uint32_t reps1 = static_cast<uint32_t>(schedule.l1) * (kMotherBitsPerBlock / 32);
        const uint32_t reps2 = static_cast<uint32_t>(schedule.l2) * (kMotherBitsPerBlock / 32);

        for (uint32_t r = 0; r < reps1; ++r) {
            for (uint32_t g = 0; g < 8; ++g) {
                uint32_t w;
                __builtin_memcpy(&w, src, sizeof(w));
                w &= p1.mask[g];
                __builtin_memcpy(dst, &w, sizeof(w));
                dst += 4;
                src += p1.keep[g];
            }
        }
        for (uint32_t r = 0; r < reps2; ++r) {
            for (uint32_t g = 0; g < 8; ++g) {
                uint32_t w;
                __builtin_memcpy(&w, src, sizeof(w));
                w &= p2.mask[g];
                __builtin_memcpy(dst, &w, sizeof(w));
                dst += 4;
                src += p2.keep[g];
            }
        }

        cursor.taken = static_cast<size_t>(src - punctured);
        cursor.written = static_cast<size_t>(dst - out);
    } else {
        for (uint16_t b = 0; b < schedule.l1; ++b)
            cursor = expand_bits(v1, kMotherBitsPerBlock, punctured, out, out_capacity, cursor);
        for (uint16_t b = 0; b < schedule.l2; ++b)
            cursor = expand_bits(v2, kMotherBitsPerBlock, punctured, out, out_capacity, cursor);
    }

    /* Tail: 1100 over 24 mother bits. */
    cursor = expand_bits(0xCCCCCCCCu, kTailMotherBits, punctured, out, out_capacity, cursor);

    return cursor.written;
}

} /* namespace dab */
