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

#ifndef DMR_VOCODER_DECODER_HPP
#define DMR_VOCODER_DECODER_HPP

#include <cstdint>

namespace dmr_vocoder {

void reset();

// 49 bits, MSB-first in 7 bytes; 160 PCM16 samples at 8 kHz.

void decode(const uint8_t* frame, int16_t* pcm);

}  // namespace dmr_vocoder
#endif
