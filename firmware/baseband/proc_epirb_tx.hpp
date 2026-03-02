/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifndef __PROC_EPIRB_TX_H__
#define __PROC_EPIRB_TX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"

class EPIRBTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    bool configured{false};

    bool end_of_transmission{};
    int8_t re{0}, im{0};         // they have sign + and -.

    // Config
    uint32_t config_pre_count = 0;
    uint32_t config_post_count = 0;

    // Data
    uint8_t frame_data[18]{0};
    uint8_t frame_data_len = 0;


    // BPSK parameters
    float phase_deg = 63.0f; // Target phase +/-63°
    float phase_rad = phase_deg * M_PI / 180.0f; // Convert to radian

    // I/Q values for BPSK
    int8_t i_pos = (int8_t)(cos(phase_rad) * 127);
    int8_t q_pos = (int8_t)(sin(phase_rad) * 127);
    int8_t i_neg = i_pos;
    int8_t q_neg = -q_pos;

    uint32_t samples_per_halfbit = 1920;  // 1536000 / 400 / 2
    uint32_t sample_counter = 0;
    uint32_t bpsk_pre_count = 0;
    uint32_t bpsk_post_count = 0;

    uint32_t bit_index = 0;
    uint32_t byte_index = 0;

    uint8_t current_byte = 0;
    uint8_t current_bit = 0;

    bool manchester_half = false; // false = first half

    TXProgressMessage txprogress_message{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{1536000, this, baseband::Direction::Transmit};
};

#endif
