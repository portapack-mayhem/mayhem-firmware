/*
 * Copyright (C) 2026 Alexey Verhogladov
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
 *
 * LoRa PHY. The format this implements - LoRa's physical layer - is not documented
 * by its vendor; what is public comes from reverse engineering, chiefly Robyns et
 * al. (2016) and the gr-lora / gr-lora_sdr projects. This frame layer follows
 * gr-lora_sdr, and it is worth being precise about how closely, since a reviewer
 * comparing them will see it:
 *
 *   - the diagonal interleaver follows gr-lora_sdr's implementation directly -
 *     same loop structure, same (i - j - 1) mod n index, same appended parity bit
 *     under reduced rate;
 *   - the explicit-header checksum uses the same equations, which are the format
 *     itself, written differently here;
 *   - the whitening is done with the LFSR rather than their lookup table, and the
 *     Hamming layer is a different code word width with different equations.
 *
 * gr-lora_sdr is GPL-3, as is this project, so this is attribution rather than a
 * licensing question.
 *
 * Verified against traffic from a stock Meshtastic node; the recorded frame that
 * pins it down is in tools/lora_bench/test_lora_framing.cpp.
 */

#ifndef __LORA_FRAMING_H__
#define __LORA_FRAMING_H__

// The LoRa frame layer: the arithmetic that turns nibbles into chips and back.
// Pure functions, no hardware, no state beyond what is passed in - which is what
// lets tools/lora_bench/test_lora_framing.cpp compile the real thing on a host
// and sweep every spreading factor, bandwidth, coding rate and length in seconds.
//
// It lives in one place because it used to live in two. The transmitter and the
// receiver each carried their own copy of the Hamming code, the interleaver, the
// whitening LFSR, the header checksum and the reduced-rate rule, and every bug in
// this app's history has been those two copies disagreeing:
//
//   * the payload coding rate was read from the preset table on one side and from
//     the packet header on the other, and blocks were cut three symbols apart;
//   * use_ldro_ was computed by the transmitter and then never read, so it coded
//     the payload at full rate while every receiver on the band expected reduced.
//
// Two implementations of one rule is the bug. One implementation cannot disagree
// with itself.
//
// The layout follows gr-lora_sdr, which was itself derived from the published
// analyses of the format (Robyns et al. 2016 and Knight's work on the PHY); it is
// re-derived here rather than copied, and verified bit-exact against traffic from
// a stock Heltec node.

#include <cstdint>
#include <cstddef>

namespace lora {

// ---- reduced rate ----------------------------------------------------------

// Low Data Rate Optimisation: mandatory once a symbol lasts longer than 16 ms,
// which is the SX126x's own rule. Under it a symbol carries SF-2 bits instead of
// SF, and the transmitter leaves the two low chips empty for the receiver to drop.
inline bool ldro_needed(uint8_t sf, uint32_t bw_hz) {
    return (static_cast<uint64_t>(1u << sf) * 1000ull) > static_cast<uint64_t>(bw_hz) * 16ull;
}

// Code words per block: SF normally, SF-2 under reduced rate.
inline int words_per_block(uint8_t sf, bool ldro) {
    return ldro ? static_cast<int>(sf) - 2 : static_cast<int>(sf);
}

// Dividing a chip by four for reduced rate ROUNDS rather than truncates. The
// transmitter sends chip = 4*value, so the receiver's job is to decide which group
// of four the bin fell in - and a real bin does not land exactly on 4*value, it
// lands near it (measured residual 0.06 to 0.14 of a bin at BW125). Truncating puts
// every symbol whose residual is slightly negative into the group below: a
// deterministic error on the symbols near a boundary and none at all on the others,
// which reads as the same wrong nibble in the same place on every packet. Adding
// half the divisor first centres the decision.
inline uint32_t ldro_div4(uint32_t chip, uint32_t n) {
    return ((chip + 2u) & (n - 1u)) >> 2u;
}

// ---- gray code -------------------------------------------------------------

inline uint32_t gray_encode(uint32_t v) {
    return v ^ (v >> 1);
}

inline int gray_decode(int g) {
    int v = g, s = g >> 1;
    while (s) {
        v ^= s;
        s >>= 1;
    }
    return v;
}

// ---- whitening -------------------------------------------------------------

// Polynomial x^8+x^6+x^5+x^4+1, initial state 0xFF, one byte per step.
// Sequence: FF FE FC F8 F0 E1 C2 85 0B ...
inline uint8_t whiten_next(uint8_t& state) {
    const uint8_t out = state;
    const uint8_t fb = ((state >> 7) ^ (state >> 5) ^ (state >> 4) ^ (state >> 3)) & 1u;
    state = static_cast<uint8_t>((state << 1) | fb);
    return out;
}

// ---- Hamming ---------------------------------------------------------------

inline void int2bits(int v, int n, uint8_t* bits) {
    for (int i = 0; i < n; i++) bits[i] = (v >> (n - 1 - i)) & 1;
}
inline int bits2int(const uint8_t* bits, int n) {
    int x = 0;
    for (int i = 0; i < n; i++) x = (x << 1) | bits[i];
    return x;
}

// cr_app = 4 for the header block (8-bit code words), 1 for CR 4/5, and so on.
inline int hamming_encode(int nibble, int cr_app) {
    uint8_t d[4];
    int2bits(nibble, 4, d);  // d[0] = MSB
    if (cr_app != 1) {
        const int p0 = d[3] ^ d[2] ^ d[1], p1 = d[2] ^ d[1] ^ d[0];
        const int p2 = d[3] ^ d[2] ^ d[0], p3 = d[3] ^ d[1] ^ d[0];
        const int full = (d[3] << 7) | (d[2] << 6) | (d[1] << 5) | (d[0] << 4) | (p0 << 3) | (p1 << 2) | (p2 << 1) | p3;
        return full >> (4 - cr_app);
    }
    const int p4 = d[0] ^ d[1] ^ d[2] ^ d[3];  // CR 4/5 is a single parity bit
    return (d[3] << 4) | (d[2] << 3) | (d[1] << 2) | (d[0] << 1) | p4;
}

// The data nibble, taken from the top four bits of the code word and reversed, with
// NO syndrome correction.
//
// Correcting looked like free error recovery and is not. Hamming(8,4) corrects one
// error; with two the syndrome points at an innocent DATA bit and breaks a nibble
// that arrived intact. Measured on a BW125 capture: the uncorrected path rebuilt the
// broadcast address as FF FF FF FF while the corrected path returned FF FF FF 7F
// from the very same symbols. When the errors land in the parity bits, as they do,
// ignoring the parity is strictly better.
inline uint8_t hamming_top4(uint32_t cw, int cw_len) {
    const uint8_t b0 = (cw >> (cw_len - 1)) & 1u, b1 = (cw >> (cw_len - 2)) & 1u;
    const uint8_t b2 = (cw >> (cw_len - 3)) & 1u, b3 = (cw >> (cw_len - 4)) & 1u;
    return static_cast<uint8_t>((b3 << 3) | (b2 << 2) | (b1 << 1) | b0);
}

// Hamming(8,4) WITH single-error correction, for the header block only - that one is
// eight symbols long, always sent at CR 4/8, and losing it loses the whole packet,
// so there the parity is worth having.
inline uint8_t hamming_correct84(uint8_t cw) {
    int d0 = (cw >> 4) & 1, d1 = (cw >> 5) & 1, d2 = (cw >> 6) & 1, d3 = (cw >> 7) & 1;
    const int rp0 = (cw >> 3) & 1, rp1 = (cw >> 2) & 1, rp2 = (cw >> 1) & 1, rp3 = cw & 1;
    const int s0 = rp0 ^ (d3 ^ d2 ^ d1), s1 = rp1 ^ (d2 ^ d1 ^ d0);
    const int s2 = rp2 ^ (d3 ^ d2 ^ d0), s3 = rp3 ^ (d3 ^ d1 ^ d0);
    switch ((s0 << 3) | (s1 << 2) | (s2 << 1) | s3) {
        case 11:
            d3 ^= 1;
            break;
        case 14:
            d2 ^= 1;
            break;
        case 13:
            d1 ^= 1;
            break;
        case 7:
            d0 ^= 1;
            break;
        default:
            break;
    }
    return static_cast<uint8_t>((d0 << 3) | (d1 << 2) | (d2 << 1) | d3);
}

// ---- interleaver -----------------------------------------------------------

// Diagonal interleave: `words` code words of cw_len bits -> cw_len chips of
// words bits. Under reduced rate a parity bit is appended and the value is shifted
// up so the two low chips stay empty, which is exactly what ldro_div4 undoes.
inline void interleave(const int* cw, int words, int cw_len, bool ldro, int sf, int n, uint16_t* out_chips) {
    uint8_t cw_bits[12][12];
    for (int k = 0; k < words; k++) int2bits(cw[k], cw_len, cw_bits[k]);
    const int nbits = words + (ldro ? 1 : 0);
    for (int i = 0; i < cw_len; i++) {
        uint8_t inter[12] = {0};
        for (int j = 0; j < words; j++)
            inter[j] = cw_bits[((i - j - 1) % words + words) % words][i];
        if (ldro) {
            int s = 0;
            for (int x = 0; x < words; x++) s += inter[x];
            inter[words] = s & 1;
        }
        const int g = bits2int(inter, nbits) << (sf - nbits);
        out_chips[i] = static_cast<uint16_t>((gray_decode(g) + 1) % n);
    }
}

// ...and back: cw_len symbols of `words` bits (MSB first) -> `words` code words of
// cw_len bits. cw[(i-j-1) mod words] bit (cw_len-1-i) = symbol i bit j.
inline void deinterleave(const uint16_t* syms, int words, int cw_len, uint16_t* cw) {
    for (int c = 0; c < words; c++) cw[c] = 0;
    for (int i = 0; i < cw_len; i++)
        for (int j = 0; j < words; j++) {
            const int bit = (syms[i] >> (words - 1 - j)) & 1u;
            const int c = ((i - j - 1) % words + words) % words;
            cw[c] = static_cast<uint16_t>(cw[c] | (bit << (cw_len - 1 - i)));
        }
}

// ---- explicit header -------------------------------------------------------

// The Semtech explicit header's five-bit checksum over its first three nibbles.
// nib[0..1] carry the payload length, nib[2] is (coding rate << 1) | has_crc,
// nib[3] holds c4 in its low bit and nib[4] the remaining four.
inline void header_checksum(uint8_t h0, uint8_t h1, uint8_t h2, uint8_t* c4, uint8_t* clo) {
    const uint8_t b4 = ((h0 >> 3) & 1) ^ ((h0 >> 2) & 1) ^ ((h0 >> 1) & 1) ^ (h0 & 1);
    const uint8_t b3 = ((h0 >> 3) & 1) ^ ((h1 >> 3) & 1) ^ ((h1 >> 2) & 1) ^ ((h1 >> 1) & 1) ^ (h2 & 1);
    const uint8_t b2 = ((h0 >> 2) & 1) ^ ((h1 >> 3) & 1) ^ (h1 & 1) ^ ((h2 >> 3) & 1) ^ ((h2 >> 1) & 1);
    const uint8_t b1 = ((h0 >> 1) & 1) ^ ((h1 >> 2) & 1) ^ (h1 & 1) ^ ((h2 >> 2) & 1) ^ ((h2 >> 1) & 1) ^ (h2 & 1);
    const uint8_t b0 = (h0 & 1) ^ ((h1 >> 1) & 1) ^ ((h2 >> 3) & 1) ^ ((h2 >> 2) & 1) ^ ((h2 >> 1) & 1) ^ (h2 & 1);
    *c4 = b4;
    *clo = static_cast<uint8_t>((b3 << 3) | (b2 << 2) | (b1 << 1) | b0);
}

// Symbols per block, and the coding rate a received header declares.
inline int header_coding_rate(uint8_t nib2) {
    return (nib2 >> 1) & 7;
}
inline bool header_has_crc(uint8_t nib2) {
    return nib2 & 1;
}

}  // namespace lora

#endif /*__LORA_FRAMING_H__*/
