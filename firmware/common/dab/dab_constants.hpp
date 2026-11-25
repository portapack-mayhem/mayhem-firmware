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

/* DAB transmission mode I parameters (ETSI EN 300 401). Modes II-IV are not
 * used terrestrially in Band III or L-Band and are not implemented. */

#ifndef __DAB_CONSTANTS_H__
#define __DAB_CONSTANTS_H__

#include <cstddef>
#include <cstdint>

namespace dab {

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

} /* namespace dab */

#endif /*__DAB_CONSTANTS_H__*/
