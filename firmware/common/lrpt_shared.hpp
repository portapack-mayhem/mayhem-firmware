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

// 72k: OQPSK at 72000 symbols/s. 80k: 80000 symbols/s, the code bits interleaved over about
// 18 s, with an 8 bit marker every 72 of them.
enum class Mode : uint8_t {
    M72K = 0,
    M80K = 1,
};

constexpr uint32_t symbol_rate(Mode mode) {
    return (mode == Mode::M80K) ? 80000 : 72000;
}
// 4 samples a symbol into the demodulator, 8 times that from the radio
constexpr uint32_t input_rate(Mode mode) {
    return symbol_rate(mode) * 4;
}
constexpr uint32_t baseband_rate(Mode mode) {
    return input_rate(mode) * 8;
}

constexpr size_t FRAME_BYTES = 1020;  // a CADU after its ASM
constexpr size_t RING_SLOTS = 6;
constexpr size_t CONSTELLATION_POINTS = 64;

// The image comes in strips of 8 rows of 196 MCUs of 8x8 pixels.
constexpr size_t STRIP_WIDTH = 1568;
constexpr size_t STRIP_ROWS = 8;
constexpr size_t PLANES = 3;  // red, green, blue
constexpr size_t PLANE_BYTES = STRIP_WIDTH * STRIP_ROWS;

// 80k interleaver: soft value n after a marker goes to branch n % 36, the receiver delays
// branch k by (35 - k) * 2048 values of that branch.
constexpr uint8_t INTER_MARKER = 0x27;
constexpr size_t INTER_MARKER_BITS = 8;
constexpr size_t INTER_PERIOD = 80;  // marker and 72 code bits
constexpr size_t INTER_DATA = INTER_PERIOD - INTER_MARKER_BITS;
constexpr size_t INTER_BRANCHES = 36;
constexpr size_t INTER_DELAY = 2048;

// The deinterleaver works in blocks of DEINT_DEPTH values of each branch, packed to
// DEINT_SOFT_BITS. The M4 hands each block to the M0, which keeps the last 18 s of them in a
// file and reads back, for each branch, the block that branch is late by.
#if defined(PORTAPACK_BOARD_PRALINE)
constexpr size_t DEINT_SOFT_BITS = 4;
constexpr size_t DEINT_DEPTH = 512;
#else
constexpr size_t DEINT_SOFT_BITS = 2;  // the M4 RAM of a HackRF One has no room for more
constexpr size_t DEINT_DEPTH = 256;
#endif
constexpr size_t DEINT_SEGMENT = DEINT_DEPTH * DEINT_SOFT_BITS / 8;  // one branch of a block
constexpr size_t DEINT_BLOCK = DEINT_SEGMENT * INTER_BRANCHES;
constexpr size_t DEINT_BLOCK_SOFTS = DEINT_DEPTH * INTER_BRANCHES;
constexpr size_t DEINT_LAG = INTER_DELAY / DEINT_DEPTH;          // blocks, from one branch to the next
constexpr size_t DEINT_FILL = (INTER_BRANCHES - 1) * DEINT_LAG;  // blocks before all branches are in
constexpr size_t DEINT_SLOTS = 2;

enum DeintState : uint8_t {
    SLOT_FREE = 0,
    SLOT_FULL = 1,     // input: the M4 filled it, the M0 has to write it
    SLOT_WRITTEN = 2,  // input: on the card, the M4 still reads branch 35 from it
    SLOT_READY = 3,    // output: the M0 read it back, the M4 may use it
};

// Lives in the M4 RAM, the M0 gets its address in each status message. It is gone once the
// baseband is shut down.
struct Shared {
    // M4 fills a slot, sets busy and tells the M0, which clears busy when done with it.
    // A frame for a busy slot is dropped, the M4 never waits.
    uint8_t frames[RING_SLOTS][FRAME_BYTES];
    volatile uint8_t busy[RING_SLOTS];
    int8_t constellation[CONSTELLATION_POINTS][2];
    volatile uint8_t mode_request;  // Mode, set by the M0

    // 80k. Blocks are numbered from 0 again each time generation changes. The M4 fills input
    // block n while it uses output block n - 1, and branch 35 of output block n - 1 is branch
    // 35 of input block n - 1, still in its slot. Slot of block n: n % DEINT_SLOTS.
    volatile uint32_t generation;
    volatile uint32_t in_seq[DEINT_SLOTS];
    volatile uint8_t in_state[DEINT_SLOTS];
    volatile uint32_t out_seq[DEINT_SLOTS];
    volatile uint8_t out_state[DEINT_SLOTS];
    volatile uint64_t out_valid[DEINT_SLOTS];  // the branches that were on the card
    volatile uint32_t next_consumed;           // first block the M4 has not started using
    uint8_t in[DEINT_SLOTS][DEINT_BLOCK];
    uint8_t out[DEINT_SLOTS][DEINT_BLOCK];

    // strip being decoded, only the M0 uses it
    uint8_t planes[PLANES][PLANE_BYTES];
};

}  // namespace lrpt

#endif /*__LRPT_SHARED_H__*/
