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

/* DAB convolutional decoder (EN 300 401 11.1): rate 1/4 mother code, K = 7,
 * generators 133, 171, 145, 133 octal, zero-terminated with 6 flush bits. The
 * state holds the 6 latest input bits, newest at bit 5, so the next state is
 * (b << 5) | (s >> 1) and traceback needs one decision bit per state per step.
 * Soft input: positive means 1, negative 0, zero an erasure. */

#ifndef __DAB_VITERBI_H__
#define __DAB_VITERBI_H__

#include "dab_soft.hpp"
#include "dab_constants.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

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
    /* One per trellis step: four codes (dab_soft.hpp), the step's first
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

#endif /*__DAB_VITERBI_H__*/
