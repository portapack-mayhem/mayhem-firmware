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

#include "decoder.hpp"
#include <cstring>

extern "C" {
#include "vocoder_asm.h"
}

namespace dmr_vocoder {

#include "portable_decoder_state.inc"

alignas(8) static uint8_t state[0x800];
static_assert(sizeof(vocoder_decoder_initial_state) == sizeof(state), "Decoder state size");

void reset() {
    std::memcpy(state, vocoder_decoder_initial_state, sizeof(state));
}

void decode(const uint8_t* frame, int16_t* pcm) {
    int16_t bits[49];

    for (unsigned i = 0; i < 49; ++i)
        bits[i] = (frame[i / 8] >> (7 - i % 8)) & 1;

    asm_decode_wav(pcm, 80, bits, 0, 0, 0, state);
    asm_decode_wav(pcm + 80, 80, bits, 0, 0, 1, state);
}

}  // namespace dmr_vocoder
