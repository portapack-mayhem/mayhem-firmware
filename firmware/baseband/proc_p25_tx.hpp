/*
 * Copyright (C) 2024 PortaPack-MAYHEM Contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROC_P25_TX_H__
#define __PROC_P25_TX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "message.hpp"

// P25 C4FM: 4800 baud, 2.4 Msps = 500 samples per dibit
#define P25_SAMPLERATE 2400000
#define P25_SAMPLES_PER_DIBIT 500

class P25TxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    bool configured{false};
    uint16_t frame_length{0};  // total dibits in frame
    uint16_t dibit_index{0};   // current dibit being transmitted
    uint32_t sample_count{0};  // samples emitted for current dibit

    // FM phase accumulator (32-bit wrapping)
    uint32_t phase{0};

    // Phase step per sample per deviation unit (600 Hz at 2.4 Msps)
    // = round(600 * 2^32 / 2400000) = 1073742
    static constexpr uint32_t base_phase_step{1073742};

    TXProgressMessage txprogress_message{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{P25_SAMPLERATE, this, baseband::Direction::Transmit};
};

#endif /* __PROC_P25_TX_H__ */
