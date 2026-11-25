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

/* DAB protection profiles and puncturing (EN 300 401 section 11). A CU carries
 * 64 * 4 / 0.096 = 2666.67 coded bit/s. EEP-A sizes are n * {12, 8, 6, 4} CU for
 * levels 1..4 at n * 8 kbit/s (rates 1/4, 3/8, 1/2, 3/4); EEP-B sizes are
 * n * {27, 21, 18, 15} CU at n * 32 kbit/s (rates 4/9, 4/7, 4/6, 4/5). */

#ifndef __DAB_PROTECTION_H__
#define __DAB_PROTECTION_H__

#include <cstddef>
#include <cstdint>

namespace dab {

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

#endif /*__DAB_PROTECTION_H__*/
