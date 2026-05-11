/*
 * NRZ-M — port of SatDump `diff::NRZMDiff`
 * (`src-core/common/codings/differential/nrzm.cpp`, MIT).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#include "meteor_nrzm.hpp"

void MeteorNrzmByteDecoder::decode(uint8_t* data, int length) {
    for (int i = 0; i < length; i++) {
        const uint8_t mask = (uint8_t)(((data[i] >> 1) & 0x7F) | (last_bit_ << 7));
        last_bit_ = (uint8_t)(data[i] & 1);
        data[i] ^= mask;
    }
}

void MeteorNrzmBitDecoder::decode_bits(uint8_t* data, int length) {
    for (int i = 0; i < length; i++) {
        const uint8_t cur = data[i];
        data[i] = (uint8_t)(cur ^ last_bit_);
        last_bit_ = cur;
    }
}
