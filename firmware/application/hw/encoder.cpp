/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

// Now supporting multiple levels of rotary encoder dial sensitivity
//
// Portapack H2 normally has a 30-step encoder, meaning one step (pulse) every
// 12 degrees of rotation.
//
// For each encoder "pulse" there are 4 state transitions, and we can choose
// between looking at all of them (high sensitivity), half of them (medium/default),
// or one quarter of them (low sensitivity).
static const int8_t transition_map[][16] = {
    // Normal (Medium) Sensitivity -- default
    {
        0,   // 0000: noop
        0,   // 0001: ccw start
        0,   // 0010: cw start
        0,   // 0011: rate
        1,   // 0100: cw end
        0,   // 0101: noop
        0,   // 0110: rate
        -1,  // 0111: ccw end
        -1,  // 1000: ccw end
        0,   // 1001: rate
        0,   // 1010: noop
        1,   // 1011: cw end
        0,   // 1100: rate
        0,   // 1101: cw start
        0,   // 1110: ccw start
        0,   // 1111: noop
    },
    // Low Sensitivity
    {
        0,   // 0000: noop
        0,   // 0001: ccw start
        0,   // 0010: cw start
        0,   // 0011: rate
        1,   // 0100: cw end
        0,   // 0101: noop
        0,   // 0110: rate
        0,   // 0111: ccw end
        -1,  // 1000: ccw end
        0,   // 1001: rate
        0,   // 1010: noop
        0,   // 1011: cw end
        0,   // 1100: rate
        0,   // 1101: cw start
        0,   // 1110: ccw start
        0,   // 1111: noop
    },
    // High Sensitivity
    {
        0,   // 0000: noop
        -1,  // 0001: ccw start
        1,   // 0010: cw start
        0,   // 0011: rate
        1,   // 0100: cw end
        0,   // 0101: noop
        0,   // 0110: rate
        -1,  // 0111: ccw end
        -1,  // 1000: ccw end
        0,   // 1001: rate
        0,   // 1010: noop
        1,   // 1011: cw end
        0,   // 1100: rate
        1,   // 1101: cw start
        -1,  // 1110: ccw start
        0,   // 1111: noop
    },
};

int_fast8_t Encoder::update(
    const uint_fast8_t phase_0,
    const uint_fast8_t phase_1) {
    state <<= 1;
    state |= phase_0;
    state <<= 1;
    state |= phase_1;

    // dial sensitivity setting is stored in pmem
    return transition_map[portapack::persistent_memory::config_encoder_dial_sensitivity()][state & 0xf];
}
