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
// how many transitions are needed before movement is registered
static const int8_t transition_map[16] = {
    0,   // 0000: noop
    -1,  // 0001: ccw start
    1,   // 0010: cw start
    0,   // 0011: rate
    1,   // 0100: cw end
    0,   // 0101: noop
    -1,  // 0110: (rate) worn decoder (missing 11 phase) workaround
    -1,  // 0111: ccw end
    -1,  // 1000: ccw end
    1,   // 1001: (rate) worn decoder (missing 11 phase) workaround
    0,   // 1010: noop
    1,   // 1011: cw end
    0,   // 1100: rate
    1,   // 1101: cw start
    -1,  // 1110: ccw start
    0,   // 1111: noop
};

int_fast8_t Encoder::update(
    const uint_fast8_t phase_0,
    const uint_fast8_t phase_1) {
    state <<= 1;
    state |= phase_0;
    state <<= 1;
    state |= phase_1;

    int_fast8_t retval = transition_map[state & 0xf];

    transition_count += retval;
    if (abs(transition_count) >= portapack::persistent_memory::config_encoder_dial_sensitivity())
        transition_count = 0;
    else
        retval = 0;        

    return retval;
}
