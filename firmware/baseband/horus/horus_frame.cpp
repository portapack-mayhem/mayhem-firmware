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

#include "horus_frame.hpp"

#include <cstring>

namespace horus {

namespace {

constexpr uint16_t kUniqueWord = 0x2424;  // "$$"
constexpr int kUwBytes = 2;
constexpr int kSizeList[] = {22, 32, 48, 64, 96, 128};

// Golay(23,12) syndrome decoding table: error pattern for every syndrome.
// Built at start-up in RAM (8 KB) rather than stored as code.
constexpr uint32_t kGolayPoly = 0xC75;
uint32_t golay_table[2048];
bool golay_ready = false;

uint32_t golay_syndrome(uint32_t c) {
    for (int x = 11; x >= 0; x--) {
        if (c & ((1u << 11) << x)) c ^= kGolayPoly << x;
    }
    return c;
}

void golay_init() {
    if (golay_ready) return;
    golay_table[0] = 0;
    for (int x = 0; x < 23; x++) {
        const uint32_t d = 1u << x;
        golay_table[golay_syndrome(d)] = d;
        for (int y = x + 1; y < 23; y++) {
            const uint32_t d2 = d | (1u << y);
            golay_table[golay_syndrome(d2)] = d2;
            for (int z = y + 1; z < 23; z++) {
                const uint32_t d3 = d2 | (1u << z);
                golay_table[golay_syndrome(d3)] = d3;
            }
        }
    }
    golay_ready = true;
}

int popcount(uint32_t v) {
    int n = 0;
    while (v) {
        v &= v - 1;
        n++;
    }
    return n;
}

// Returns the corrected codeword (data in bits 22..11); counts corrected bits.
uint32_t golay_decode(uint32_t c, int& corrected) {
    const uint32_t e = golay_table[golay_syndrome(c)];
    corrected += popcount(e);
    return c ^ e;
}

bool is_prime(uint32_t x) {
    for (uint32_t i = 2; i * i <= x; i++)
        if (x % i == 0) return false;
    return x > 1;
}

uint32_t interleaver_b(int nbytes) {
    if (nbytes == 43) return 337;  // horus v1
    if (nbytes == 63) return 389;  // horus v2
    const uint32_t nbits = (uint32_t)nbytes * 8;
    uint32_t b = (nbits * 100u) / 162u;  // floor(nbits / 1.62)
    do {
        b++;
    } while (!is_prime(b));
    return b;
}

// Inverse of the transmitter's algebraic interleaver (dir = 1 in horus_l2.c).
void deinterleave(uint8_t* inout, uint8_t* out, int nbytes) {
    const uint32_t nbits = (uint32_t)nbytes * 8;
    const uint32_t b = interleaver_b(nbytes);
    memset(out, 0, nbytes);
    for (uint32_t n = 0; n < nbits; n++) {
        const uint32_t i = (b * n) % nbits;  // read position
        const uint32_t j = n;                // write position
        const uint32_t ibit = (inout[i / 8] >> (i % 8)) & 1u;
        out[j / 8] |= ibit << (j % 8);
    }
    memcpy(inout, out, nbytes);
}

// Additive scrambler; the same operation scrambles and descrambles.
void scramble(uint8_t* inout, int nbytes) {
    uint16_t sc = 0x4a80;
    const int nbits = nbytes * 8;
    for (int i = 0; i < nbits; i++) {
        const uint16_t so = ((sc & 0x2) >> 1) ^ (sc & 0x1);
        const int byte = i / 8, shift = i % 8;
        const uint8_t bit = ((inout[byte] >> shift) & 1u) ^ so;
        inout[byte] = (inout[byte] & ~(1u << shift)) | (bit << shift);
        sc >>= 1;
        sc |= so << 14;
    }
}

// Port of horus_l2_decode_rx_packet(): rx holds UW + data bits + parity bits.
int golay_unpack(uint8_t* out, const uint8_t* rx, int npayload) {
    const int ndata_bits = npayload * 8;
    const uint8_t* pin = rx + kUwBytes + npayload;
    int corrected = 0;
    uint32_t ingolay = 0;
    int ningolay = 0, nparity = 0, noutbits = 0;
    uint8_t outbyte = 0;
    uint8_t paritybyte = *pin++;
    uint8_t* pout = out;

    for (int ninbit = 0; ninbit < ndata_bits; ninbit++) {
        const int inbit = (rx[ninbit / 8 + kUwBytes] >> (7 - (ninbit % 8))) & 1;
        ingolay = (ingolay | inbit) << 1;
        if ((++ningolay % 12) == 0) {
            for (int i = 0; i < 11; i++) {
                ingolay |= (paritybyte >> (7 - (nparity % 8))) & 1u;
                if (i != 10) ingolay <<= 1;
                if ((++nparity % 8) == 0) paritybyte = *pin++;
            }
            const uint32_t data = golay_decode(ingolay, corrected) >> 11;
            for (int i = 0; i < 12; i++) {
                outbyte |= (data >> (11 - i)) & 1u;
                if (++noutbits % 8)
                    outbyte <<= 1;
                else {
                    *pout++ = outbyte;
                    outbyte = 0;
                }
            }
            ingolay = 0;
        }
    }

    if (ningolay % 12) {  // final, partially filled codeword
        uint32_t parity = 0;
        for (int i = 0; i < 11; i++) {
            parity |= (paritybyte >> (7 - (nparity % 8))) & 1u;
            if (i != 10) parity <<= 1;
            if ((++nparity % 8) == 0) paritybyte = *pin++;
        }
        ingolay >>= 1;
        const uint32_t data = golay_decode((ingolay << 12) + parity, corrected) >> 11;
        const int ntogo = ndata_bits - noutbits;
        for (int i = 0; i < ntogo; i++) {
            outbyte |= (data >> (ntogo - i)) & 1u;
            if (++noutbits % 8)
                outbyte <<= 1;
            else {
                *pout++ = outbyte;
                outbyte = 0;
            }
        }
    }
    return corrected;
}

}  // namespace

int horus_num_tx_bytes(int payload_bytes) {
    const int data_bits = payload_bytes * 8;
    const int codewords = (data_bits + 11) / 12;
    const int tx_bits = kUwBytes * 8 + data_bits + codewords * 11;
    return (tx_bits + 7) / 8;
}

uint16_t horus_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        uint8_t x = (crc >> 8) ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ (uint16_t)x;
    }
    return crc;
}

Framer::Framer() {
    golay_init();
    reset();
}

void Framer::reset() {
    nbits_ = 0;
    uw_reg_ = 0;
    for (auto& c : cand_) c.next = -1;
}

bool Framer::push_symbol(uint8_t sym) {
    const bool a = push_bit((sym >> 1) & 1u);  // MSB first
    const bool b = push_bit(sym & 1u);
    return a || b;
}

bool Framer::push_bit(uint8_t bit) {
    bits_[nbits_ & (kBitBuf - 1)] = bit;
    nbits_++;
    uw_reg_ = (uw_reg_ << 1) | bit;

    if (nbits_ >= 16 && popcount(uw_reg_ ^ kUniqueWord) <= 1) {
        uw_hits_++;
        for (auto& c : cand_) {
            if (c.next < 0) {
                c.start = nbits_ - 16;
                c.next = 0;
                break;
            }
        }
    }

    for (auto& c : cand_) {
        if (c.next < 0) continue;
        const uint32_t need = (uint32_t)horus_num_tx_bytes(kSizeList[c.next]) * 8;
        if (nbits_ - c.start < need) continue;
        if (try_decode(c.start, c.next)) {
            // drop every candidate that started inside this frame
            const uint32_t end = c.start + need;
            for (auto& o : cand_) {
                if (o.next >= 0 && o.start < end) o.next = -1;
            }
            return true;
        }
        if (++c.next >= kSizes) c.next = -1;
    }
    return false;
}

bool Framer::try_decode(uint32_t start, int size_idx) {
    const int npayload = kSizeList[size_idx];
    const int ntx = horus_num_tx_bytes(npayload);

    for (int i = 0; i < ntx; i++) {
        uint8_t v = 0;
        for (int j = 0; j < 8; j++) v = (v << 1) | bit_at(start + i * 8 + j);
        rx_[i] = v;
    }
    scramble(&rx_[kUwBytes], ntx - kUwBytes);
    deinterleave(&rx_[kUwBytes], tmp_, ntx - kUwBytes);
    const int corrected = golay_unpack(packet_.data, rx_, npayload);

    uint8_t version = 0;
    const uint8_t* d = packet_.data;
    if (npayload == 22 || npayload == 32) {
        // v1 / v2: CRC over everything but the last two bytes, stored little endian at the end
        const uint16_t crc = horus_crc16(d, npayload - 2);
        if (crc == (uint16_t)(d[npayload - 2] | (d[npayload - 1] << 8))) version = (npayload == 22) ? 1 : 2;
    }
    if (!version && npayload != 22) {
        // v3: CRC first, over the rest
        const uint16_t crc = horus_crc16(d + 2, npayload - 2);
        if (crc == (uint16_t)(d[0] | (d[1] << 8))) version = 3;
    }
    if (!version) return false;

    packet_.size = (uint8_t)npayload;
    packet_.version = version;
    packet_.bits_corrected = (uint8_t)(corrected > 255 ? 255 : corrected);
    return true;
}

}  // namespace horus
