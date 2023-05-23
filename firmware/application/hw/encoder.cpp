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


// Now supporting 3 levels of rotary encoder dial sensitivity
//
// Portapack H2 normally has a 30-step encoder, meaning one step (pulse) every
// 12 degrees of rotation.
//
// For each encoder "pulse" there are 4 state transitions, and we can choose
// between looking at all of them (high sensitivity), half of them (medium/default),
// or one quarter of them (low sensitivity).
//
// Each non-zero nibble in the transition_map represents an encoder state transition
// that we interpret as either a 1 or -1.
// Clockwise state transitions are:  00b->10b, 10b->11b, 11b->01b, 01b->00b.
//                             i.e.  2, B, D, 4
// Counter-clockwise transitions are: 00b->01b, 01b->11b, 11b->10b, 10b->00b.
//                             i.e.  1, 7, E, 8
//  e.g. 0x0B = 1011b means input transition from 10b->11b, which means clockwise +1.
//       Notice that B is not in transition_map[0] so this transition is ignored when
//       in "low sensitivity" mode.
//
// Four no-op states (no change) are 00b->00b, 01b->01b, 10b->10b, and 11b->11b,
// (AKA 0, 5, A, and F), which are of course ignored since no rotation occurred.
// Four additional non-zero values may be seen when the encoder dial is turned
// rapidly and intermediate states are skipped 00b->11b, 01b->10b, 10b->01b, and 11b->00b
// (AKA 3, 6, 9, or C); these are also (currently) ignored.
//
static const uint32_t transition_map_[NUM_DIAL_SENSITIVITY] = {
    0x00070004,  // low sensitivity
    0x008700B4,  // medium sensitivity
    0xE187D2B4   // high sensitivity
};

int_fast8_t Encoder::update(
    const uint_fast8_t phase_0,
    const uint_fast8_t phase_1) {  
    state <<= 1;
    state |= phase_0;
    state <<= 1;
    state |= phase_1;

    uint32_t tmap = transition_map_[sensitivity_];
    do {
        if ((tmap & 0x0F) == (state & 0x0F))
            return 1;
        if (((tmap >> 16) & 0x0F) == (state & 0x0F))
            return -1;
        tmap >>= 4;
    } while ((tmap >> 16) != 0);

    return 0;
}
