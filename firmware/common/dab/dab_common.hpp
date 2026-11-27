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

/* Shared by both cores: frame constants, the fixed spec readings, the FIB
 * CRC and superframe firecode, and the protection profiles the FIC
 * describes and the MSC depunctures with. */

#ifndef __DAB_COMMON_H__
#define __DAB_COMMON_H__

#include <cstddef>
#include <cstdint>

namespace dab {

/* ---- Frame constants ---- */

/* DAB transmission mode I parameters (ETSI EN 300 401). Modes II-IV are not
 * used terrestrially in Band III or L-Band and are not implemented. */

constexpr uint32_t kSampleRate = 2048000;
constexpr size_t kFftSize = 2048;      // useful symbol part, 1 ms
constexpr size_t kGuardSamples = 504;  // guard interval, 246 us
constexpr size_t kSymbolSamples = kFftSize + kGuardSamples;
constexpr size_t kNullSamples = 2656;  // null symbol, 1.297 ms
constexpr size_t kCarriers = 1536;     // active carriers, 1 kHz spacing

/* Carrier positions run in ascending frequency, DC excluded: position 0 is
 * carrier -768, 767 is -1, 768 is +1 and 1535 is +768. */
constexpr int16_t carrier_at_position(size_t position) {
    return (position < kCarriers / 2)
               ? static_cast<int16_t>(static_cast<int>(position) - 768)
               : static_cast<int16_t>(static_cast<int>(position) - 767);
}

constexpr size_t carrier_position(int carrier) {
    return (carrier < 0) ? static_cast<size_t>(carrier + 768) : static_cast<size_t>(carrier + 767);
}

/* Frame layout: null, TFPR phase reference, 3 FIC symbols, 72 MSC symbols. */
constexpr size_t kFicSymbols = 3;
constexpr size_t kMscSymbols = 72;
constexpr size_t kSymbolsPerFrame = 1 + kFicSymbols + kMscSymbols;  // excluding null
constexpr uint32_t kFrameDurationMs = 96;

/* Common Interleaved Frame. */
constexpr size_t kCifsPerFrame = 4;
constexpr size_t kCuPerCif = 864;
constexpr size_t kBitsPerCu = 64;
constexpr size_t kBitsPerCif = kCuPerCif * kBitsPerCu;

/* The widest subchannel decoded, per CIF: 72 CU, 96 kbit/s at EEP 3-A. The
 * baseband sizes its largest buffers from these (~2.3 KB of M4 heap per 512
 * coded bits) and refuses anything wider, so the app offers nothing wider.
 * The payload cap is separate: EEP 4-A carries 3,456 bits in 72 CU. */
constexpr size_t kMaxCodedBitsPerCif = 4608;
constexpr size_t kMaxPayloadBitsPerCif = 2304;

/* Fast Information Channel. */
constexpr size_t kFibsPerFrame = 12;
constexpr size_t kFibBytes = 32;
constexpr size_t kFibPayloadBytes = 30;

/* Convolutional code: rate 1/4 mother code, constraint length 7. */
constexpr size_t kConstraintLength = 7;
constexpr size_t kViterbiStates = 1u << (kConstraintLength - 1);

/* DAB+ superframe: five logical frames, RS(120,110) code words. */
constexpr size_t kSuperframeLogicalFrames = 5;
constexpr size_t kRsBlockLength = 120;
constexpr size_t kRsDataLength = 110;

/* ---- Fixed spec readings ---- */

/* Spec readings where the wrong choice decodes to noise rather than an error,
 * each fixed at the reading that decodes. decode_variant is reported in status. */

/* Symbol after the null the FIC starts on. DQPSK is differential, so a frame
 * one symbol out still looks locked; only the FIC decode fails. */
constexpr uint8_t fic_offset = 1;

/* Integer carrier offset, biased by kCarrierShiftBias. Guard correlation cannot
 * see a whole-carrier shift: the frame locks with every carrier in the wrong bin. */
constexpr int16_t kCarrierShiftBias = 4;
constexpr uint8_t carrier_shift = kCarrierShiftBias;

constexpr uint8_t decode_variant = 0;

constexpr bool variant_freq_inverse() {
    return false;
}

/* ---- CRC-16 and firecode ---- */

/* FIB CRC-16 and DAB+ superframe firecode. Bitwise: a few runs per frame do
 * not justify a 512-byte table in the M4's tight code region. */

/* Poly 0x1021, init 0xFFFF, no final inversion; "123456789" -> 0x29B1. */
uint16_t crc16_ccitt(const uint8_t* data, size_t length);

/* Covers the 30 payload bytes; stored inverted, big-endian, in bytes 30-31. */
bool fib_crc_ok(const uint8_t fib[32]);
void fib_crc_stamp(uint8_t fib[32]);

/* (16,11) shortened Fire code, generator 0x782F, over superframe bytes 2..10,
 * stored in bytes 0..1. */
bool firecode_ok(const uint8_t superframe_head[11]);
void firecode_stamp(uint8_t superframe_head[11]);

/* ---- Protection profiles and depuncturing ---- */

/* DAB protection profiles and puncturing (EN 300 401 section 11). A CU carries
 * 64 * 4 / 0.096 = 2666.67 coded bit/s. EEP-A sizes are n * {12, 8, 6, 4} CU for
 * levels 1..4 at n * 8 kbit/s (rates 1/4, 3/8, 1/2, 3/4); EEP-B sizes are
 * n * {27, 21, 18, 15} CU at n * 32 kbit/s (rates 4/9, 4/7, 4/6, 4/5). */

enum class ProtectionKind : uint8_t {
    Eep,
    Uep,
};

struct ProtectionProfile {
    ProtectionKind kind{ProtectionKind::Eep};
    uint8_t level{3};      // 1..4
    bool option_b{false};  // false = EEP-A, true = EEP-B

    bool operator==(const ProtectionProfile& other) const {
        return kind == other.kind && level == other.level && option_b == other.option_b;
    }
};

uint16_t eep_cu_per_unit(const ProtectionProfile& profile);

/* bit/s; 0 if the size is not a whole number of units, which rejects a corrupt
 * FIG 0/1 rather than giving a plausible wrong rate. */
uint32_t eep_bitrate(uint16_t size_cu, const ProtectionProfile& profile);

/* Data bits per CIF. */
uint32_t eep_subchannel_bits(uint16_t size_cu, const ProtectionProfile& profile);

/* PI_1..PI_24, first mother bit in the MSB. PI_i keeps i + 8 of the 32 bits:
 * rate 8/9 (PI_1) to 1/4 (PI_24, unpunctured). 0 out of range. */
uint32_t puncturing_vector(uint8_t index);

uint8_t puncturing_weight(uint8_t index);

/* Writes 0 (erasure) where a bit was removed; returns mother bits written. */
size_t depuncture(const int8_t* punctured,
                  size_t punctured_length,
                  uint32_t pi_vector,
                  int8_t* out,
                  size_t out_capacity);

} /* namespace dab */

#endif /*__DAB_COMMON_H__*/
