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

/* Channel decoding (M4): packed soft bits, energy dispersal, time
 * interleaving, FIC and MSC depuncturing and the Viterbi decoder. */

#ifndef __DAB_FEC_H__
#define __DAB_FEC_H__

#include "dab_common.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

/* ---- Packed soft bits ---- */

/* Packed soft bits. The byte trellis reduces every int8 value to
 * clamp(v >> 5, +-3) anyway, so reducing at the demapper's copy loses nothing
 * and saves ~12.7 KB at 72 CU, enough for a 96 kbit/s subchannel. A code is the
 * level as 4-bit two's complement, 0 the erasure, packed two a byte (index i in
 * byte i / 2, low nibble first); the trellis takes a uint16 per step. */

constexpr int kSoftShift = 5;
constexpr int kSoftMax = 3;

inline uint8_t soft_code(int8_t v) {
    int32_t q = static_cast<int32_t>(v) >> kSoftShift;
    if (q < -kSoftMax)
        q = -kSoftMax;
    if (q > kSoftMax)
        q = kSoftMax;
    return static_cast<uint8_t>(q) & 0x0Fu;
}

inline int32_t soft_level(uint32_t code) {
    return static_cast<int32_t>(code << 28) >> 28;
}

inline uint8_t soft_get(const uint8_t* run, size_t i) {
    return (i & 1) ? static_cast<uint8_t>(run[i >> 1] >> 4) : static_cast<uint8_t>(run[i >> 1] & 0x0Fu);
}

/* Four int8 soft values (byte k of `w`) to four codes (nibble k), all lanes at
 * once. A byte's top three bits are v >> 5 as a 3-bit code; the one clamp,
 * -4 to -3, is a zero-lane test on t ^ 4 (x + 7 sets bit 3 unless x is 0);
 * bit 2 copied to bit 3 sign-extends; one shift folds lanes 1, 3 into 0, 2.
 * Checked against soft_code() on all 2^32 words. */
inline uint32_t soft_codes4(uint32_t w) {
    static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "byte k of the word is value k");
    uint32_t t = (w >> 5) & 0x07070707u;
    const uint32_t live = ((t ^ 0x04040404u) + 0x07070707u) & 0x08080808u;
    t |= (~live & 0x08080808u) >> 3;
    t |= (t & 0x04040404u) << 1;
    t |= t >> 4;
    return (t & 0xFFu) | ((t >> 8) & 0xFF00u);
}

/* `values` int8 soft values (a multiple of 4) to a packed code run: the copy
 * every CIF symbol of the subchannel takes into msc_in_. */
inline void pack_codes(const int8_t* src, uint8_t* dst, size_t values) {
    for (size_t n = values / 4; n != 0; --n, src += 4, dst += 2) {
        uint32_t w;
        __builtin_memcpy(&w, src, sizeof(w));
        const uint32_t r = soft_codes4(w);
        dst[0] = static_cast<uint8_t>(r);
        dst[1] = static_cast<uint8_t>(r >> 8);
    }
}

/* ---- Energy dispersal ---- */

/* DAB energy dispersal (EN 300 401 section 10): PRBS x^9 + x^5 + 1, register
 * initialised to all ones. Scrambling and descrambling are the same XOR.
 * The sequence begins 0x07 0xBE 0x2E 0x64. */

class PrbsGenerator {
   public:
    PrbsGenerator() { reset(); }

    void reset() { register_ = 0x1FF; }

    uint8_t next_bit() {
        /* Output the feedback bit: the shifted-out bit is the same m-sequence
         * delayed by nine, which passes structural tests and descrambles to noise. */
        const uint8_t feedback = ((register_ >> 8) ^ (register_ >> 4)) & 1;
        register_ = static_cast<uint16_t>(((register_ << 1) | feedback) & 0x1FF);
        return feedback;
    }

    uint8_t next_byte() {
        uint8_t v = 0;
        for (int i = 0; i < 8; ++i)
            v = static_cast<uint8_t>((v << 1) | next_bit());
        return v;
    }

   private:
    uint16_t register_{0x1FF};
};

/* In place, from the sequence's initial state. */
void prbs_descramble(uint8_t* data, size_t length);

/* ---- Time interleaving ---- */

/* DAB MSC time deinterleaving (EN 300 401 12.1): bit i of a CIF's subchannel is
 * on branch i % 16, delayed bitreverse4(branch) CIFs at the transmitter and the
 * complementary 15 - that here, so every branch lines up 15 CIFs late. Storage
 * is 7.5 three-bit codes (packed soft bits, above) per coded bit. The FIC is not time
 * interleaved. */

/* Branches must divide the CIF evenly. */
constexpr size_t kInterleaverBranches = 16;

/* Total delay applied across transmitter and receiver, in CIFs. */
constexpr size_t kInterleaverLatencyCifs = 15;

class TimeDeinterleaver {
   public:
    TimeDeinterleaver() = default;

    /* `bits_per_cif` must be a non-zero multiple of 16, at most
     * kMaxCodedBitsPerCif. `storage` holds storage_bytes_for() bytes, outlives
     * this, and on the M4 is not the AHB "USB" bank, which overlaps M0 RAM. */
    bool configure(size_t bits_per_cif, uint8_t* storage, size_t storage_bytes);

    /* Codes at three bits, eight to three bytes. Each branch row is padded to
     * whole groups of eight; the slack after the last row is never read.
     * configure() refuses a smaller buffer. */
    static constexpr size_t row_slots(size_t bits_per_cif) {
        return ((bits_per_cif / kInterleaverBranches) + 7) & ~static_cast<size_t>(7);
    }
    static constexpr size_t storage_bytes_for(size_t bits_per_cif) {
        return row_slots(bits_per_cif) * 120 / 8 * 3;
    }

    /* One packed CIF, in place: on return `cif` holds the deinterleaved
     * codes. False for the first 15 calls, while the pipeline fills. */
    bool push_packed(uint8_t* cif);

    void reset();

    size_t bits_per_cif() const { return bits_per_cif_; }

    /* Transmitter-side delay for a branch, in CIFs. Exposed so tests can
     * build the matching interleaver. */
    static uint8_t branch_delay(uint8_t branch);

   private:
    size_t bits_per_cif_{0};
    size_t per_branch_{0};
    size_t row_{0};
    size_t offset_[kInterleaverBranches]{};
    size_t head_[kInterleaverBranches]{};
    uint8_t depth_[kInterleaverBranches]{};
    size_t pushed_{0};

    uint8_t* storage_{nullptr};
    size_t storage_bytes_{0};

    /* Advances every branch's head; true once the pipeline has filled. */
    inline bool end_cif();
};

/* ---- FIC depuncturing ---- */

/* FIC depuncturing (EN 300 401 section 11.2): 768 payload bits (three FIBs)
 * plus a 6-bit tail give 3096 mother bits, punctured to 2304, three symbols:
 * 21 blocks x 4 x 24 (PI_16) + 3 x 4 x 23 (PI_15) + 12 of the 24-bit tail. */

constexpr size_t kFicPayloadBits = 768;

/* 4 * (768 + 6) */
constexpr size_t kFicMotherBits = 3096;

constexpr size_t kFicPuncturedBits = 2304;

/* 12 FIBs per frame in groups of 3. */
constexpr size_t kFicCodewordsPerFrame = 4;

/* Writes 0 (erasure) where a bit was removed; returns mother bits written. */
size_t depuncture_fic(const int8_t* punctured, int8_t* out, size_t out_capacity);

/* Into the packed trellis input (packed soft bits, above), one step word per four mother
 * bits. Returns mother bits written. */
size_t depuncture_fic_packed(const int8_t* punctured, uint16_t* out, size_t out_capacity_steps);

/* ---- MSC subchannels ---- */

/* MSC depuncturing (EN 300 401 section 11.3): L1 blocks of 128 mother bits with
 * PI(v+1), L2 with PI(v), then a 24-bit tail keeping 12. Derived, not tabled:
 * L1 + L2 = payload bits / 32 and L1*w(v+1) + L2*w(v) = (capacity - 12) / 4
 * solve uniquely, as adjacent vectors differ in weight by one. A schedule that
 * does not balance (corrupt FIG 0/1, unsupported profile) is rejected. */

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

/* Packed (packed soft bits, above): `punctured` is a code run, `out` one word per trellis
 * step. Returns mother bits written. Groups are fetched as words: `punctured`
 * must be readable four bytes past its last code. */
size_t depuncture_msc_packed(const MscSchedule& schedule,
                             const uint8_t* punctured,
                             uint16_t* out,
                             size_t out_capacity_steps);

/* ---- Viterbi decoder ---- */

/* DAB convolutional decoder (EN 300 401 11.1): rate 1/4 mother code, K = 7,
 * generators 133, 171, 145, 133 octal, zero-terminated with 6 flush bits. The
 * state holds the 6 latest input bits, newest at bit 5, so the next state is
 * (b << 5) | (s >> 1) and traceback needs one decision bit per state per step.
 * Soft input: positive means 1, negative 0, zero an erasure. */

/* Largest payload per call: a FIC codeword (768 bits) or a CIF of the widest
 * subchannel. Traceback is a sliding window, so state does not grow. */
constexpr size_t kViterbiMaxPayloadBits = kMaxPayloadBitsPerCif;

/* Survivor depth: over twice the 5*(K-1) rule of thumb. 64 rather than 96
 * because the block traceback walks depth + block states per block, and it
 * keeps the ring a power of two. */
constexpr size_t kViterbiTracebackDepth = 64;

/* Bits recovered per traceback. Tracing once per block costs
 * (depth + block) / block walk steps per bit instead of depth. */
constexpr size_t kViterbiOutputBlock = 64;

class ViterbiDecoder {
   public:
    /* One per trellis step: four codes (packed soft bits, above), the step's first
     * mother bit in the low nibble. */
    using Soft = uint16_t;
    static constexpr size_t kSoftPerStep = 1;

    ViterbiDecoder() { reset(); }

    void reset();

    /* Resumable: a codeword takes ~22 ms and the DMA ring holds 4 ms. advance()
     * runs at most `max_steps` steps and returns true once complete. */
    bool begin(const Soft* soft_in, size_t coded_bits, uint8_t* out_bytes, size_t out_capacity_bytes);
    bool advance(size_t max_steps);
    size_t payload_bits() const { return payload_bits_; }
    bool busy() const { return busy_; }

    /* One-call decode into packed bytes, MSB first. `coded_bits` must be
     * 4 * (payload + 6), payload at most kViterbiMaxPayloadBits. Returns the
     * payload bits written, or 0 on inconsistent arguments. */
    size_t decode(const Soft* soft_in,
                  size_t coded_bits,
                  uint8_t* out_bytes,
                  size_t out_capacity_bytes);

   private:
    static constexpr size_t kStates = kViterbiStates;  // 64
    static constexpr size_t kDepth = kViterbiTracebackDepth;
    static constexpr size_t kBlock = kViterbiOutputBlock;

    /* The ring must hold the survivor walk plus the block read out of it. */
    static constexpr size_t kRing = kDepth + kBlock;
    static constexpr size_t kRingMask = kRing - 1;
    static_assert((kRing & kRingMask) == 0, "traceback ring must be a power of two");

    /* Unsigned byte scores, maximised; the offset making branch metrics
     * non-negative is common to every state, so it never changes a comparison. */
    using metric_t = uint8_t;

    /* Reads the caller's live metric buffer: advance() ping-pongs between two. */
    size_t best_state(const metric_t* m) const;
    static size_t predecessor(size_t state, uint64_t decisions);

    const Soft* in_{nullptr};
    uint8_t* out_{nullptr};
    size_t steps_total_{0};
    size_t step_{0};
    size_t payload_bits_{0};
    bool busy_{false};

    /* Word aligned: the trellis reads and writes metrics four at a time. */
    alignas(4) metric_t metric_[kStates];
    alignas(4) metric_t next_metric_[kStates];
    uint64_t decision_[kRing];
    size_t emitted_{0};
};

/* Test encoder: `in_bits` payload bits (packed MSB first) to one byte per coded
 * bit, with the 6-bit zero flush. Returns the coded bits written. */
size_t convolutional_encode(const uint8_t* in_bytes, size_t in_bits, uint8_t* out_bits);

/* Branch output for a 7-bit register value: generator 133 in bit 3 down to the
 * second 133 in bit 0. Exposed for testing. */
uint8_t convolutional_output(uint8_t register_value);

} /* namespace dab */

#endif /*__DAB_FEC_H__*/
