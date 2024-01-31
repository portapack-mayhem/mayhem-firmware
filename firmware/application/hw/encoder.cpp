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
static const int8_t transition_map[] = {
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
};

int_fast8_t Encoder::update(
    const uint_fast8_t phase_0,
    const uint_fast8_t phase_1) {
    state = (state << 2) | (phase_0 << 1) | phase_1;

    int8_t direction = transition_map[state & 0xf];

    // store only valid transitions
    if (direction) {
        store <<= 4;
        store |= state & 0x0f;

        // dial sensitivity setting is stored in pmem
        switch (portapack::persistent_memory::config_encoder_dial_sensitivity()) {
            case 0:  // Normal (Medium) Sensitivity -- default
                switch (store & 0xff) {
                    case 0x2b:
                    case 0xd4:
                        direction = 1;
                        break;
                    case 0x17:
                    case 0xe8:
                        direction = -1;
                        break;
                    default:
                        direction = 0;
                        break;
                }
                break;
            case 1:  // Low Sensitivity
                switch (store & 0xff) {
                    case 0x2b:
                        direction = 1;
                        break;
                    case 0x17:
                        direction = -1;
                        break;
                    default:
                        direction = 0;
                        break;
                }
                break;
            case 2:  // High Sensitivity
                switch (store & 0xff) {
                    case 0x2b:
                    case 0xd4:
                    case 0xbd:
                    case 0x42:
                        direction = 1;
                        break;
                    case 0x17:
                    case 0xe8:
                    case 0x7e:
                    case 0x81:
                        direction = -1;
                        break;
                    default:
                        direction = 0;
                        break;
                }
                break;
        }
    }
    return direction;
}
