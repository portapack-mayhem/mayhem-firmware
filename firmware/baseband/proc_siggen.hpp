/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __PROC_SIGGEN_H__
#define __PROC_SIGGEN_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"

class SigGenProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    bool configured{false};

    uint32_t tone_delta{0}, fm_delta{}, tone_phase{0};
    uint8_t tone_shape{}, modulation{};
    uint32_t sample_count{0};
    bool auto_off{};
    int32_t phase{0}, sphase{0}, delta{0};  // they may have sign in the pseudo random sample generation.
    int8_t sample{0}, re{0}, im{0};         // they have sign + and -.
    uint16_t seed_value_16 = {0xACE1};      // seed 16 bits lfsr : any nonzero start state will work.
    uint16_t lfsr_16{}, bit_16{};           // bit must be 16-bit to allow bit<<15 later in the code */
    uint8_t counter{0};
    // uint8_t seed_value = {0x56}; 					// Finally not used lfsr of 8 bits , seed 8blfsr : any nonzero start state will work.
    // uint8_t lfsr { }, bit { };  						// Finally not used lfsr of 8 bits , bit must be 8-bit to allow bit<<7 later in the code */

    TXProgressMessage txprogress_message{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{1536000, this, baseband::Direction::Transmit};
};

#endif
