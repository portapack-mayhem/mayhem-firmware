/*
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

#ifndef __LRPT_SHARED_H__
#define __LRPT_SHARED_H__

// Meteor-M LRPT receiver (lrpt_rx): what the M4 and the M0 share.

#include <cstddef>
#include <cstdint>

namespace lrpt {

constexpr uint32_t SYMBOL_RATE = 72000;
constexpr size_t FRAME_BYTES = 1020;  // a CADU after its ASM
constexpr size_t RING_SLOTS = 8;
constexpr size_t CONSTELLATION_POINTS = 64;

// The image comes in strips of 8 rows of 196 MCUs of 8x8 pixels.
constexpr size_t STRIP_WIDTH = 1568;
constexpr size_t STRIP_ROWS = 8;
constexpr size_t PLANES = 3;  // red, green, blue
constexpr size_t PLANE_BYTES = STRIP_WIDTH * STRIP_ROWS;

// Lives in the M4 RAM, the M0 gets its address in each status message. It is gone once the
// baseband is shut down.
struct Shared {
    // M4 fills a slot, sets busy and tells the M0, which clears busy when done with it.
    // A frame for a busy slot is dropped, the M4 never waits.
    uint8_t frames[RING_SLOTS][FRAME_BYTES];
    volatile uint8_t busy[RING_SLOTS];
    int8_t constellation[CONSTELLATION_POINTS][2];
    // strip being decoded, only the M0 uses it
    uint8_t planes[PLANES][PLANE_BYTES];
};

}  // namespace lrpt

#endif /*__LRPT_SHARED_H__*/
