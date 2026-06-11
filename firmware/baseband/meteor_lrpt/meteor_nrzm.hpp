/*
 * NRZ-M decoders matching SatDump `diff::NRZMDiff`
 * (`src-core/common/codings/differential/nrzm.cpp`, Lucas Teske / Aang23; MIT):
 *   - `MeteorNrzmByteDecoder::decode` == `NRZMDiff::decode` (legacy LRPT after Viterbi bytes).
 *   - `MeteorNrzmBitDecoder::decode_bits` == `NRZMDiff::decode_bits` (M2-x: one byte per bit 0/1).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#ifndef __METEOR_NRZM_HPP__
#define __METEOR_NRZM_HPP__

#include <cstdint>

class MeteorNrzmByteDecoder {
   public:
    void reset() { last_bit_ = 0; }

    void decode(uint8_t* data, int length);

   private:
    uint8_t last_bit_{0};
};

class MeteorNrzmBitDecoder {
   public:
    void reset() { last_bit_ = 0; }

    /* length = number of bit-symbols (each byte should be 0 or 1). */
    void decode_bits(uint8_t* data, int length);

   private:
    uint8_t last_bit_{0};
};

#endif
