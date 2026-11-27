/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#ifndef DMR_AMBE_FEC_H
#define DMR_AMBE_FEC_H

#include <stdint.h>

#ifdef __cplusplus

extern "C" {
#endif

// 72 on-air bits -> 49 codec bits, MSB first. Returns corrected A+B bits,
// or -1 for an uncorrectable extended Golay A word. C has no protection.

int dmr_ambe_decode(const uint8_t channel[9], uint8_t codec[7]);
uint32_t dmr_ambe_golay24(uint16_t data);
uint32_t dmr_ambe_scramble(uint16_t data);

#ifdef __cplusplus
}

#endif
#endif
