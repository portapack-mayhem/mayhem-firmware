/*
 * Copyright (C) 2026 HTotoo
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

#ifndef __PROC_RTTY_TX_H__
#define __PROC_RTTY_TX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"
#include <array>

class RTTYTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    static constexpr uint32_t baseband_fs = 2048000;

    bool configured{false};

    // RTTY Configuration
    uint32_t samples_per_bit{0};

    // Stop bits configuration: 2=1.0, 3=1.5, 4=2.0
    uint8_t configured_stop_bits{2};

    // FSK State
    uint32_t delta_mark{0};     // Phase step for Mark
    uint32_t delta_space{0};    // Phase step for Space
    uint32_t current_delta{0};  // Smoothed delta
    uint32_t slew_rate{0};      // Max change per sample
    uint32_t phase{0};          // Phase accumulator

    // Precision Timing
    uint32_t baud_phase{0};
    uint32_t baud_phase_increment{0};
    uint32_t base_baud_phase_increment{0};  // Store the standard 1.0 bit rate

    // State Machine
    enum class State {
        Idle,
        LeadIn,
        StartBit,
        DataBits,
        StopBit,
        LeadOut
    };
    State state{State::Idle};

    uint32_t lead_counter{0};
    uint8_t current_char{0};
    uint8_t bit_pos{0};

    // Ring Buffer
    std::array<uint8_t, 1024> data_buffer{};
    volatile size_t head{0};
    volatile size_t tail{0};

    void configure(uint16_t baud, uint16_t shift, int16_t mark_tone_, int16_t space_tone_, uint8_t stop_bits_, bool inverted_);
    void advance_state();

    bool buffer_push(uint8_t byte);
    bool buffer_pop(uint8_t& byte);
    bool buffer_empty() const;

    TXProgressMessage txprogress_message{};

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Transmit};
};

#endif /* __PROC_RTTY_TX_H__ */