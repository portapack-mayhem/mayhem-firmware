/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __PROC_OOK_H__
#define __PROC_OOK_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

class OOKProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const p) override;

   private:
    bool configured = false;

    uint32_t samples_per_bit{0};
    uint8_t repeat{0};
    uint32_t length{0};
    uint32_t pause{0};
    uint8_t de_bruijn_length{0};

    uint32_t pause_counter{0};
    uint8_t repeat_counter{0};
    uint8_t s{0};
    uint16_t bit_pos{0};
    uint8_t cur_bit{0};
    uint32_t sample_count{0};
    uint32_t tone_phase{0}, phase{0}, sphase{0};
    int32_t tone_sample{0}, sig{0}, frq{0};

    TXProgressMessage txprogress_message{};

    static constexpr auto MAX_DE_BRUIJN_ORDER = 24;
    uint8_t v[MAX_DE_BRUIJN_ORDER];
    uint8_t v_tmp[MAX_DE_BRUIJN_ORDER];
    unsigned int idx{0};
    unsigned int k{0};
    unsigned int duval_symbols{0};
    unsigned int duval_length{0};
    unsigned int duval_bit{0};
    unsigned int duval_sample_bit{0};
    unsigned int duval_symbol{0};
    size_t scan_progress{0};
    uint8_t scan_done{true};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{2280000, this, baseband::Direction::Transmit};

    size_t duval_algo_step();
    void scan_process(const buffer_c8_t& buffer);
    bool scan_init(unsigned int order);
    bool scan_encode(const buffer_c8_t& buffer, size_t& buf_ptr);
    void write_sample(const buffer_c8_t& buffer, uint8_t bit_value, size_t i);
};

#endif
