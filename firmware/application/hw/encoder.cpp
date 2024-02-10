/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2024 Mark Thompson
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

#include "encoder.hpp"

#include "utility.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

// Transition map for rotary encoder phase bits
//  00 -> 01 -> 11 -> 10 cw
//  00 -> 10 -> 11 -> 01 ccw
// NB: Bits are swapped versus older FW versions
static const int8_t transition_map[16] = {
    0,   // 00->00: noop
    1,   // 00->01: cw start
    -1,  // 00->10: ccw start
    0,   // 00->11: rate
    -1,  // 01->00: ccw end
    0,   // 01->01: noop
    0,   // 01->10: rate
    1,   // 01->11: cw end
    1,   // 10->00: cw end
    0,   // 10->01: rate
    0,   // 10->10: noop
    -1,  // 10->11: ccw end
    0,   // 11->00: rate
    -1,  // 11->01: ccw start
    1,   // 11->10: cw start
    0,   // 11->11: noop
};

// Rotary encoder dial sensitivity (transition ignored if bit is 0)
static const uint16_t sensitivity_map[] = {
    0x0990,  // DIAL_SENSITIVITY_NORMAL
    0x0110,  // DIAL_SENSITIVITY_LOW
    0x6996,  // DIAL_SENSITIVITY_HIGH
};

int_fast8_t Encoder::update(const uint_fast8_t phase_bits) {
    state = ((state << 2) | phase_bits) & 0x0F;

    int_fast8_t direction = transition_map[state];

    // Require 2 state changes in same direction to register movement -- for additional level of contact switch debouncing
    if (direction == prev_direction) {
        if ((sensitivity_map[portapack::persistent_memory::encoder_dial_sensitivity()] & (1 << state)) == 0)
            return 0;
        return direction;
    }

    // It's normal for transition map to return 0 between every +1/-1, so discarding those
    if (direction != 0)
        prev_direction = direction;

    return 0;
}
