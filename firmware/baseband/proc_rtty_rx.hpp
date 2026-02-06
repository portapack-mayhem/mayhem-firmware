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

#pragma once

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "dsp_fir_taps.hpp"

class RTTYRxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 4000000;

    // RTTY Config
    uint16_t baud_rate = 4545;  // Stored as baud * 100 (e.g. 4545 = 45.45 baud)
    uint16_t shift_hz = 170;
    bool configured = false;

    // DSP Config
    // 4MHz / 8 = 500kHz
    // 500kHz / 8 = 62.5kHz final sample rate
    static constexpr uint32_t decim_0_out_fs = baseband_fs / 8;
    static constexpr uint32_t decim_1_out_fs = decim_0_out_fs / 8;

    // Filter Constants (for 62.5kHz sampling)
    // DC Block Alpha ~1/64 (Shift 6)
    static constexpr int32_t DC_ALPHA_SHIFT = 6;
    // LPF Alpha ~1/16 (Shift 4) -> Cutoff ~600Hz, safe for 170Hz shift + edges
    static constexpr int32_t LPF_ALPHA_SHIFT = 4;
    static constexpr int32_t DISCRIM_GAIN = 128;

    // Decimators (Using 4k25 narrowband filters)
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    // Intermediate Buffer
    std::array<complex16_t, 512> dst_buffer_data{};
    const buffer_c16_t dst_buffer{dst_buffer_data.data(), dst_buffer_data.size()};

    // Output Message Buffer
    RTTYDataMessage tx_message{};

    // FM Demodulation State
    complex16_t sample_prev = {0, 0};
    int32_t fm_dc_val = 0;        // For DC blocking (tuning error compensation)
    int32_t fm_val_smoothed = 0;  // Low pass filtered value

    // UART / Slicer State
    enum UartState {
        WAIT_START,
        WAIT_MID_START,
        READ_BITS,
        WAIT_STOP
    };

    UartState uart_state = WAIT_START;
    uint32_t samples_per_bit = 0;
    uint32_t phase_counter = 0;
    uint8_t bit_counter = 0;
    uint8_t shift_reg = 0;

    void process_demodulated_sample(int32_t sample);
    void append_data(uint8_t raw_baudot_code);

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};