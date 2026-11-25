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

/* Spec readings where the wrong choice decodes to noise rather than an error,
 * each fixed at the reading that decodes. decode_variant is reported in status. */

#ifndef __DAB_VARIANT_H__
#define __DAB_VARIANT_H__

#include <cstdint>

namespace dab {

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

} /* namespace dab */

#endif /*__DAB_VARIANT_H__*/
