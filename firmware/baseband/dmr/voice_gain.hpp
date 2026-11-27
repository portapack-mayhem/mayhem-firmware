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

#ifndef DMR_VOICE_GAIN_HPP
#define DMR_VOICE_GAIN_HPP

#include <cstdint>

// +12 dB PCM boost before interpolation; retain hardware volume control.
// Multiply in 32 bits (left-shifting negative PCM would be undefined).

inline int16_t dmr_voice_gain(int16_t sample) {
    const int32_t amplified = static_cast<int32_t>(sample) * 4;
    return amplified > 32767 ? 32767 : amplified < -32768 ? -32768
                                                          : amplified;
}

#endif
