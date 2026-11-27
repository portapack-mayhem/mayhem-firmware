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

#include "dab_fec.hpp"

namespace dab {

/* ---- Energy dispersal ---- */

void prbs_descramble(uint8_t* data, size_t length) {
    PrbsGenerator generator;
    for (size_t i = 0; i < length; ++i)
        data[i] ^= generator.next_byte();
}

/* ---- Time interleaving ---- */

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

/* ---- FIC depuncturing ---- */

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
 * PI15 ends in 1100, the tail is 1100. Must match depuncture_fic(). */
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

/* ---- MSC subchannels ---- */

namespace {

/* kMotherBitsPerBlock and kTailMotherBits are the FIC's, above. */
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

/* ---- Viterbi decoder ---- */

namespace {

constexpr uint8_t kGenerators[4] = {0133 & 0x7F, 0171 & 0x7F, 0145 & 0x7F, 0133 & 0x7F};

/* Everything below derives from the soft-value range, the packed codes'
 * alphabet (dab_fec.hpp), and the static_asserts are the whole safety
 * argument for 8-bit metrics: change kByteSoft and the rest re-derives, or
 * the build stops. */
static constexpr int32_t kByteSoft = kSoftMax;
static constexpr int32_t kByteOffset = 4 * kByteSoft;  // makes a score non-negative
static constexpr int32_t kByteSpan = 8 * kByteSoft;    // widest a step can move a metric
static constexpr uint32_t kByteSpanWord = static_cast<uint32_t>(2 * kByteOffset) * 0x01010101u;
static constexpr uint32_t kByteDecBits = 0x08040201u;  // one bit per lane, lane 0 lowest

/* A K=7 trellis cannot spread its surviving metrics by much more than (K-1)
 * branch metrics. The bound sizes the renormalisation period; the real spread
 * reaches a little past it (156 against 144), so the ceiling is then set as
 * high as the period allows rather than at the bound. */
static constexpr int32_t kByteSpread = 6 * kByteSpan;

/* Renormalise often enough that the floor plus a period's growth stays inside
 * a byte with a little to spare, and no oftener: the scan is the one part of
 * the step that is not the butterfly. */
static constexpr int32_t kByteHeadroom = 252;
static constexpr size_t kByteRenormPeriod = (kByteSpread + 8 * kByteSpan <= kByteHeadroom)   ? 8
                                            : (kByteSpread + 4 * kByteSpan <= kByteHeadroom) ? 4
                                            : (kByteSpread + 2 * kByteSpan <= kByteHeadroom) ? 2
                                                                                             : 1;

/* Where renormalisation puts the maximum; states further below saturate to
 * zero, behind any surviving path. */
static constexpr uint32_t kByteCeil = kByteHeadroom - kByteRenormPeriod * kByteSpan;

/* The lead state 0 needs to stay ahead until the trellis forgets its start. */
static constexpr uint8_t kByteStart = static_cast<uint8_t>(kByteSpread + 1);

static_assert(kByteCeil + kByteRenormPeriod * kByteSpan <= 255,
              "byte metrics would overflow between renormalisations");
static_assert(kByteStart + kByteRenormPeriod * kByteSpan <= 255,
              "the starting lead would overflow before the first renormalisation");
static_assert(kByteCeil >= static_cast<uint32_t>(kByteSpan),
              "the renormalisation floor is too low to be useful");

inline uint8_t parity7(uint8_t v) {
    v ^= static_cast<uint8_t>(v >> 4);
    v ^= static_cast<uint8_t>(v >> 2);
    v ^= static_cast<uint8_t>(v >> 1);
    return static_cast<uint8_t>(v & 1);
}

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "decision_half() needs the halves in address order");

/* Half `h` of a decision word as one scaled load, not an LDRD and a 64-bit
 * shift. memcpy, not a cast, which is an aliasing violation -O3 may act on. */
inline uint32_t decision_half(const uint64_t* word, size_t h) {
    uint32_t v;
    __builtin_memcpy(&v, reinterpret_cast<const uint8_t*>(word) + 4 * h, sizeof(v));
    return v;
}

inline uint8_t get_bit(const uint8_t* bytes, size_t index) {
    return static_cast<uint8_t>((bytes[index >> 3] >> (7 - (index & 7))) & 1);
}

/* set_bit() without the clear: begin() zeroed the output and the traceback
 * writes each bit once. The flush still needs set_bit(). */
inline void or_bit(uint8_t* bytes, size_t index, uint8_t value) {
    bytes[index >> 3] |= static_cast<uint8_t>(value << (7 - (index & 7)));
}

inline void set_bit(uint8_t* bytes, size_t index, uint8_t value) {
    const uint8_t mask = static_cast<uint8_t>(1u << (7 - (index & 7)));
    if (value)
        bytes[index >> 3] |= mask;
    else
        bytes[index >> 3] &= static_cast<uint8_t>(~mask);
}

}  // namespace

uint8_t convolutional_output(uint8_t register_value) {
    uint8_t out = 0;
    for (int j = 0; j < 4; ++j)
        out = static_cast<uint8_t>((out << 1) | parity7(static_cast<uint8_t>(register_value & kGenerators[j])));
    return out;
}

size_t convolutional_encode(const uint8_t* in_bytes, size_t in_bits, uint8_t* out_bits) {
    if (in_bytes == nullptr || out_bits == nullptr)
        return 0;

    uint8_t state = 0;
    size_t written = 0;
    const size_t total = in_bits + kConstraintLength - 1;

    for (size_t i = 0; i < total; ++i) {
        const uint8_t b = (i < in_bits) ? get_bit(in_bytes, i) : 0;
        const uint8_t reg = static_cast<uint8_t>((b << 6) | state);
        const uint8_t out = convolutional_output(reg);

        for (int j = 0; j < 4; ++j)
            out_bits[written++] = static_cast<uint8_t>((out >> (3 - j)) & 1);

        state = static_cast<uint8_t>(reg >> 1);
    }

    return written;
}

void ViterbiDecoder::reset() {
    /* The encoder starts from state 0. A competing path can gain at most
     * (K-1) branch metrics on it before the trellis forgets its start, so a
     * lead of kByteStart is enough; unreachable states sit at zero. */
    metric_[0] = kByteStart;
    for (size_t s = 1; s < kStates; ++s)
        metric_[s] = 0;
}

size_t ViterbiDecoder::predecessor(size_t state, uint64_t decisions) {
    /* Select the 32-bit half first: a variable 64-bit shift is five
     * instructions on a 32-bit core. */
    const uint32_t half = (state < kStates / 2)
                              ? static_cast<uint32_t>(decisions)
                              : static_cast<uint32_t>(decisions >> 32);
    const uint8_t which = static_cast<uint8_t>((half >> (state & 31)) & 1);
    return ((state & 0x1F) << 1) | which;
}

bool ViterbiDecoder::begin(const Soft* soft_in,
                           size_t coded_bits,
                           uint8_t* out_bytes,
                           size_t out_capacity_bytes) {
    busy_ = false;

    if (soft_in == nullptr || out_bytes == nullptr)
        return false;
    if (coded_bits == 0 || (coded_bits % 4) != 0)
        return false;

    const size_t steps = coded_bits / 4;
    if (steps < kConstraintLength - 1)
        return false;

    const size_t payload = steps - (kConstraintLength - 1);
    if (payload == 0 || payload > kViterbiMaxPayloadBits)
        return false;
    if (out_capacity_bytes < (payload + 7) / 8)
        return false;

    reset();
    emitted_ = 0;

    for (size_t i = 0; i < (payload + 7) / 8; ++i)
        out_bytes[i] = 0;

    in_ = soft_in;
    out_ = out_bytes;
    steps_total_ = steps;
    step_ = 0;
    payload_bits_ = payload;
    busy_ = true;
    return true;
}

/* Byte-lane add-compare-select, four butterflies to a SIMD word. Soft values
 * in +-3 give branch metrics in +-12, offset by kByteOffset to 0..24. A shift
 * of 5 decodes like full precision and 6 does not. The adds
 * saturate, so a wrong spread bound degrades the decode instead of wrapping. */

#if defined(DAB_PACKED_ACS_PORTABLE) || !defined(__ARM_FEATURE_SIMD32)
/* Plain-C equivalents for a host with no ARM SIMD. They must match the
 * instructions exactly: unsigned 8-bit lanes, GE meaning "first >= second". */
static inline uint32_t byte_lane(uint32_t v, int k) {
    return (v >> (8 * k)) & 0xFFu;
}

static inline uint32_t dsp_uqadd8(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    for (int k = 0; k < 4; ++k) {
        uint32_t x = byte_lane(a, k) + byte_lane(b, k);
        if (x > 255u) x = 255u;
        r |= x << (8 * k);
    }
    return r;
}

static inline uint32_t dsp_uqsub8(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    for (int k = 0; k < 4; ++k) {
        const uint32_t x = byte_lane(a, k);
        const uint32_t y = byte_lane(b, k);
        r |= (x > y ? x - y : 0u) << (8 * k);
    }
    return r;
}

static inline uint32_t dsp_usub8(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    for (int k = 0; k < 4; ++k)
        r |= ((byte_lane(a, k) - byte_lane(b, k)) & 0xFFu) << (8 * k);
    return r;
}

/* USUB8, two SELs and a USAD8: the winner, and the lanes where the odd
 * predecessor won collapsed into a nibble. A tie keeps the even
 * predecessor. */
static inline uint32_t byte_acs(uint32_t a, uint32_t b, uint32_t& best) {
    uint32_t m = 0, n = 0;
    for (int k = 0; k < 4; ++k) {
        const uint32_t x = byte_lane(a, k);
        const uint32_t y = byte_lane(b, k);
        const bool ge = x >= y;
        m |= (ge ? x : y) << (8 * k);
        if (!ge) n |= 1u << k;
    }
    best = m;
    return n;
}

static inline uint32_t dsp_umax8(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    for (int k = 0; k < 4; ++k) {
        const uint32_t x = byte_lane(a, k);
        const uint32_t y = byte_lane(b, k);
        r |= (x >= y ? x : y) << (8 * k);
    }
    return r;
}

/* Which lanes of `a` are at least `b`, one bit each, lane 0 lowest. */
static inline uint32_t byte_ge_mask(uint32_t a, uint32_t b) {
    uint32_t n = 0;
    for (int k = 0; k < 4; ++k)
        if (byte_lane(a, k) >= byte_lane(b, k)) n |= 1u << k;
    return n;
}

static inline uint32_t dsp_uxtb16(uint32_t a) {
    return a & 0x00FF00FFu;
}
static inline uint32_t dsp_uxtb16_ror8(uint32_t a) {
    const uint32_t r = (a >> 8) | (a << 24);
    return r & 0x00FF00FFu;
}
#else
static inline uint32_t dsp_uqadd8(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm("uqadd8 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static inline uint32_t dsp_uqsub8(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm("uqsub8 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static inline uint32_t dsp_usub8(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm("usub8 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b) : "cc");
    return r;
}

/* All four instructions read or write the GE flags, so they stay in one asm
 * block; split, the compiler may schedule a flag-setter between them. */
static inline uint32_t byte_acs(uint32_t a, uint32_t b, uint32_t& best) {
    /* The compare's result reuses the winner's register once it has set the
     * flags; the early-clobber keeps it off the inputs. USAD8 is here because
     * at the call site the compiler hoisted all sixteen SELs and spilled. */
    uint32_t m, d;
    __asm(
        "usub8 %0, %2, %3\n\t"
        "sel   %0, %2, %3\n\t"
        "sel   %1, %4, %5\n\t"
        "usad8 %1, %1, %4"
        : "=&r"(m), "=&r"(d)
        : "r"(a), "r"(b), "r"(0u), "r"(kByteDecBits)
        : "cc");
    best = m;
    return d;
}

static inline uint32_t dsp_umax8(uint32_t a, uint32_t b) {
    uint32_t r;
    __asm(
        "usub8 %0, %1, %2\n\t"
        "sel   %0, %1, %2"
        : "=&r"(r)
        : "r"(a), "r"(b)
        : "cc");
    return r;
}

/* Which lanes of `a` are at least `b`, one bit each, lane 0 lowest. One asm
 * block, since all three instructions touch the GE flags. */
static inline uint32_t byte_ge_mask(uint32_t a, uint32_t b) {
    uint32_t n;
    __asm(
        "usub8 %0, %1, %2\n\t"
        "sel   %0, %3, %4\n\t"
        "usad8 %0, %0, %4"
        : "=&r"(n)
        : "r"(a), "r"(b), "r"(kByteDecBits), "r"(0u)
        : "cc");
    return n;
}

static inline uint32_t dsp_uxtb16(uint32_t a) {
    uint32_t r;
    __asm("uxtb16 %0, %1" : "=r"(r) : "r"(a));
    return r;
}

static inline uint32_t dsp_uxtb16_ror8(uint32_t a) {
    uint32_t r;
    __asm("uxtb16 %0, %1, ror #8" : "=r"(r) : "r"(a));
    return r;
}
#endif

/* memcpy, not a cast: a uint32_t* into the uint8_t metric array is an
 * aliasing violation -O3 may act on. */
static inline uint32_t byte_load32(const uint8_t* p) {
    uint32_t v;
    __builtin_memcpy(&v, p, sizeof(v));
    return v;
}

static inline void byte_store32(uint8_t* p, uint32_t v) {
    __builtin_memcpy(p, &v, sizeof(v));
}

static inline void byte_store16(uint8_t* p, uint16_t v) {
    __builtin_memcpy(p, &v, sizeof(v));
}

/* A quad's four winners are next states 4q..4q+3. In the split layout the
 * two even ones are adjacent and so are the two odd ones, so each half is one
 * halfword store. UXTB16 pulls lanes 0 and 2 (or 1 and 3) into halfwords and
 * the shift closes the gap. */
static inline uint16_t byte_pack_even(uint32_t v) {
    const uint32_t x = dsp_uxtb16(v);
    return static_cast<uint16_t>(x | (x >> 8));
}

static inline uint16_t byte_pack_odd(uint32_t v) {
    const uint32_t x = dsp_uxtb16_ror8(v);
    return static_cast<uint16_t>(x | (x >> 8));
}

/* The largest of the 64 metrics, by a UMAX8 chain unrolled over all sixteen
 * words: as a loop, -Os spent half of each iteration on loop overhead. */
static_assert(kViterbiStates == 64, "byte_max() assumes 16 metric words");
__attribute__((always_inline)) static inline uint32_t byte_max(const uint8_t* m) {
    uint32_t mx = byte_load32(m);
#define DAB_BYTE_MAX(k) mx = dsp_umax8(mx, byte_load32(m + 4 * (k)))
    DAB_BYTE_MAX(1);
    DAB_BYTE_MAX(2);
    DAB_BYTE_MAX(3);
    DAB_BYTE_MAX(4);
    DAB_BYTE_MAX(5);
    DAB_BYTE_MAX(6);
    DAB_BYTE_MAX(7);
    DAB_BYTE_MAX(8);
    DAB_BYTE_MAX(9);
    DAB_BYTE_MAX(10);
    DAB_BYTE_MAX(11);
    DAB_BYTE_MAX(12);
    DAB_BYTE_MAX(13);
    DAB_BYTE_MAX(14);
    DAB_BYTE_MAX(15);
#undef DAB_BYTE_MAX
    mx = dsp_umax8(mx, mx >> 16);
    mx = dsp_umax8(mx, mx >> 8);
    return mx & 0xFFu;
}

size_t ViterbiDecoder::best_state(const metric_t* m) const {
    /* The winner four states at a time: the maximum, then one USUB8/SEL/USAD8
     * per word for the lanes holding it. Ties go to the first index. */
    const uint32_t topw = byte_max(m) * 0x01010101u;

    size_t best = 0;
    for (size_t k = 0; k < kStates / 4; ++k) {
        const uint32_t at = byte_ge_mask(byte_load32(m + 4 * k), topw);
        if (at != 0) {
            best = 4 * k + static_cast<size_t>(__builtin_ctz(at));
            break;
        }
    }
    /* The split layout keeps even states in the low half and odd in the high,
     * so the index is not the state. */
    best = (best < kStates / 2) ? (2 * best) : (2 * (best - kStates / 2) + 1);
    return best;
}

bool ViterbiDecoder::advance(size_t max_steps) {
    if (!busy_)
        return true;

    const Soft* soft_in = in_;
    uint8_t* out_bytes = out_;
    const size_t payload_bits = payload_bits_;

    /* metric_ holds the metrics between calls. Within one, cur and nxt swap
     * each step and metric_ is reconciled on the way out. */
    metric_t* cur = metric_;
    metric_t* nxt = next_metric_;

    /* Locals, not members: nothing the loop calls reads them, and -Os would
     * otherwise reload and store them every step. */
    size_t step = step_;
    const size_t steps_total = steps_total_;

    size_t done = 0;
    while (step < steps_total && done < max_steps) {
        const size_t t = step;

        const uint32_t codes = soft_in[t];

        /* Correlation metric: agreeing with a confident soft value scores
         * high, an erasure (0) scores nothing either way. */

        uint32_t dec_lo = 0;
        uint32_t dec_hi = 0;

        /* Next states ns and ns+32 share predecessors 2i, 2i+1 and differ only
         * in register bit 6, set in every generator, so their branch metrics
         * are negatives. In the split layout (even state s at s/2, odd at
         * 32 + s/2) one word per half aligns four butterflies' predecessors.
         * convolutional_output(2i) per quad takes four patterns,
         *     q 0,3  [ 0,  9,  2, 11]     q 4,7  [ 6, 15,  4, 13]
         *     q 1,2  [13,  4, 15,  6]     q 5,6  [11,  2,  9,  0]
         * each (offset -+ s1) + (-+ (s0+s3)) + (-+ s2) with per-lane signs, a
         * multiply by sum(sign_k * 256^k); lanes stay in 0..24, so nothing
         * carries: 32 branch metrics in four multiplies and eight adds. */
        {
            /* Already reduced to the byte alphabet: four sign extends. */
            const int32_t s0 = static_cast<int32_t>(codes << 28) >> 28;
            const int32_t s1 = static_cast<int32_t>(codes << 24) >> 28;
            const int32_t s2 = static_cast<int32_t>(codes << 20) >> 28;
            const int32_t s3 = static_cast<int32_t>(codes << 16) >> 28;

            /* s0 and s3 always carry the same sign in every pattern above, so
             * they travel together. */
            const uint32_t pu = static_cast<uint32_t>(s0 + s3) * 0x00FF00FFu;  // (-,+,-,+)
            const uint32_t pv = static_cast<uint32_t>(s2) * 0x0100FEFFu;       // (-,-,+,+)
            const uint32_t wp = static_cast<uint32_t>(kByteOffset - s1) * 0x01010101u;
            const uint32_t wm = static_cast<uint32_t>(kByteOffset + s1) * 0x01010101u;

            /* Each pattern's complement, 2*kByteOffset minus it, feeds the high
             * half of every butterfly and is derived per group below; building
             * it with plain adds is bit-identical but makes -Os spill more. */
            const uint32_t pa = wp + pu + pv;
            const uint32_t pb = wm - pu + pv;
            const uint32_t pc = wm + pu - pv;
            const uint32_t pd = wp - pu - pv;

            const uint8_t* __restrict ew = cur;
            const uint8_t* __restrict ow = cur + kStates / 2;
            uint8_t* __restrict ne = nxt;
            uint8_t* __restrict no = nxt + kStates / 2;

/* Quad q writes next states 4q..4q+3 from the low half and 32+4q..32+4q+3 from
 * the high half: split-layout indices 2q, 2q+1 (evens) and 32+2q, 32+2q+1
 * (odds), two halfword stores per half. */
#define DAB_BYTE_QUAD(q, bp, bn)                                           \
    {                                                                      \
        const uint32_t e = byte_load32(ew + 4 * (q));                      \
        const uint32_t o = byte_load32(ow + 4 * (q));                      \
        uint32_t best;                                                     \
        dec_lo |= byte_acs(dsp_uqadd8(e, (bp)), dsp_uqadd8(o, (bn)), best) \
                  << (4 * (q));                                            \
        byte_store16(ne + 2 * (q), byte_pack_even(best));                  \
        byte_store16(no + 2 * (q), byte_pack_odd(best));                   \
        dec_hi |= byte_acs(dsp_uqadd8(e, (bn)), dsp_uqadd8(o, (bp)), best) \
                  << (4 * (q));                                            \
        byte_store16(ne + kStates / 4 + 2 * (q), byte_pack_even(best));    \
        byte_store16(no + kStates / 4 + 2 * (q), byte_pack_odd(best));     \
    }

/* Quads sharing a pattern go together (0/3, 1/2, 4/7, 5/6) so only one pattern
 * and its complement are live; in index order -Os spilled. Quads write disjoint
 * metrics and decision bits, so the order does not change the result. */
#define DAB_BYTE_GROUP(qa, qb, bpe)                       \
    {                                                     \
        const uint32_t bp = (bpe);                        \
        const uint32_t bn = dsp_usub8(kByteSpanWord, bp); \
        DAB_BYTE_QUAD(qa, bp, bn)                         \
        DAB_BYTE_QUAD(qb, bp, bn)                         \
    }
            DAB_BYTE_GROUP(0, 3, pa)
            DAB_BYTE_GROUP(1, 2, pb)
            DAB_BYTE_GROUP(4, 7, pc)
            DAB_BYTE_GROUP(5, 6, pd)
#undef DAB_BYTE_GROUP
#undef DAB_BYTE_QUAD
        }

        decision_[t & kRingMask] = static_cast<uint64_t>(dec_lo) |
                                   (static_cast<uint64_t>(dec_hi) << 32);

        /* Renormalise against a ceiling: a saturating subtract pulls the
         * maximum down to kByteCeil, which bounds the top and floors states
         * too far behind to matter at zero. Unsigned metrics cannot subtract
         * the maximum, and subtracting the minimum leaves the top unbounded. */
        if ((t & (kByteRenormPeriod - 1)) == (kByteRenormPeriod - 1)) {
            const uint32_t top = byte_max(nxt);
            if (top > kByteCeil) {
                const uint32_t sub = (top - kByteCeil) * 0x01010101u;
                /* Unrolled, as in byte_max(). */
#define DAB_BYTE_RENORM(k) \
    byte_store32(nxt + 4 * (k), dsp_uqsub8(byte_load32(nxt + 4 * (k)), sub))
                DAB_BYTE_RENORM(0);
                DAB_BYTE_RENORM(1);
                DAB_BYTE_RENORM(2);
                DAB_BYTE_RENORM(3);
                DAB_BYTE_RENORM(4);
                DAB_BYTE_RENORM(5);
                DAB_BYTE_RENORM(6);
                DAB_BYTE_RENORM(7);
                DAB_BYTE_RENORM(8);
                DAB_BYTE_RENORM(9);
                DAB_BYTE_RENORM(10);
                DAB_BYTE_RENORM(11);
                DAB_BYTE_RENORM(12);
                DAB_BYTE_RENORM(13);
                DAB_BYTE_RENORM(14);
                DAB_BYTE_RENORM(15);
#undef DAB_BYTE_RENORM
            }
        }

        /* Swap rather than copy; reconciled once when advance() returns. */
        metric_t* const tmp = cur;
        cur = nxt;
        nxt = tmp;

        /* Block traceback: every kBlock steps, walk the survivor back through
         * the depth and read the next kBlock bits out of the same walk. */
        if (((t + 1) % kBlock) == 0 && t + 1 >= kRing) {
            size_t state = best_state(cur);
            const size_t last_emitted = t - kDepth;

            /* predecessor() inlined: state >> 5 is both the decision half and
             * the decoded bit, so one scaled LDR replaces the LDRD, compare and
             * moves. */
            const size_t stop = t + 1 - kRing;
            for (size_t i = t;; --i) {
                const size_t half = state >> 5;
                const uint32_t dec = decision_half(&decision_[i & kRingMask], half);

                if (i <= last_emitted && i < payload_bits)
                    or_bit(out_bytes, i, static_cast<uint8_t>(half));

                state = ((state & 0x1F) << 1) | ((dec >> (state & 31)) & 1);

                if (i == 0 || i <= stop)
                    break;
            }

            emitted_ = last_emitted + 1;
        }
        ++step;
        ++done;
    }
    step_ = step;

    /* An odd number of swaps leaves the live metrics in next_metric_. */
    if (cur != metric_) {
        for (size_t s = 0; s < kStates; ++s)
            metric_[s] = cur[s];
    }

    if (step < steps_total)
        return false;

    /* Flush whatever the last block did not cover. The codeword is
     * zero-terminated, so the survivor is known to end in state 0 and the
     * remaining bits can be read straight off. */
    size_t state = 0;
    for (size_t t = steps_total_; t-- > emitted_;) {
        const uint8_t bit = static_cast<uint8_t>((state >> 5) & 1);
        if (t < payload_bits_)
            set_bit(out_bytes, t, bit);
        state = predecessor(state, decision_[t & kRingMask]);
    }

    busy_ = false;
    return true;
}

size_t ViterbiDecoder::decode(const Soft* soft_in,
                              size_t coded_bits,
                              uint8_t* out_bytes,
                              size_t out_capacity_bytes) {
    if (!begin(soft_in, coded_bits, out_bytes, out_capacity_bytes))
        return 0;

    while (!advance(coded_bits));

    return payload_bits_;
}

} /* namespace dab */
