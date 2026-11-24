/*
 * Copyright (C) 2026 OE1LBS
 *
 * Frame decoding ported from horusdemodlib (horus_l2.c, golay23.c),
 * Copyright (C) David Rowe, Mark Jessop and contributors, LGPL-2.1.
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

/*
 * Horus Binary frame layer: finds the "$$" unique word in the demodulated bit
 * stream and tries every known payload size (v1: 22, v2/v3: 32/48/64/96/128 bytes).
 * Each candidate is descrambled, de-interleaved and Golay(23,12) decoded, then
 * accepted only if its CRC16 matches (at the end for v1/v2, at the start for v3).
 */

#ifndef __HORUS_FRAME_H__
#define __HORUS_FRAME_H__

#include <cstdint>
#include <cstddef>

namespace horus {

constexpr size_t kMaxPayload = 128;

struct Packet {
    uint8_t data[kMaxPayload];
    uint8_t size;            // payload bytes, including the CRC
    uint8_t version;         // 1, 2 or 3
    uint8_t bits_corrected;  // Golay-corrected bit errors in the frame
};

class Framer {
   public:
    Framer();
    void reset();

    // Feed one 4FSK symbol (0..3). Returns true when a packet passed its CRC;
    // it is then available from packet().
    bool push_symbol(uint8_t sym);
    const Packet& packet() const { return packet_; }
    uint32_t uw_hits() const { return uw_hits_; }  // unique words seen (diagnostics)

   private:
    static constexpr int kSizes = 6;
    static constexpr uint32_t kBitBuf = 2048;  // power of two, > longest frame (1992 bits)
    static constexpr int kMaxCandidates = 8;

    struct Candidate {
        uint32_t start;  // absolute bit index of the unique word
        int8_t next;     // next size index to try, -1 = free slot
    };

    bool push_bit(uint8_t bit);
    bool try_decode(uint32_t start, int size_idx);
    uint8_t bit_at(uint32_t abs) const { return bits_[abs & (kBitBuf - 1)]; }

    uint8_t bits_[kBitBuf];
    uint32_t nbits_ = 0;
    uint16_t uw_reg_ = 0;
    uint32_t uw_hits_ = 0;
    Candidate cand_[kMaxCandidates];

    uint8_t rx_[256];   // coded frame bytes
    uint8_t tmp_[256];  // interleaver scratch
    Packet packet_{};
};

// Exposed for tests.
int horus_num_tx_bytes(int payload_bytes);
uint16_t horus_crc16(const uint8_t* data, size_t len);

}  // namespace horus

#endif /*__HORUS_FRAME_H__*/
