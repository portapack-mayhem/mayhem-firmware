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

#ifndef __PROC_RTTY_RX_H__
#define __PROC_RTTY_RX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "audio_output.hpp"
#include "dsp_fir_taps.hpp"

class RTTYRxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;

    // RTTY Config
    uint16_t baud_rate = 0;  // 0 = Auto-detect
    uint16_t shift_hz = 170;
    bool configured = false;

    // DSP Rates
    // Stage 0: 3.072M / 8 = 384k
    // Stage 1: 384k / 8   = 48k
    // Stage 2: 48k / 2    = 24k (Audio/Demod)
    static constexpr uint32_t decim_0_out_fs = baseband_fs / 8;
    static constexpr uint32_t decim_1_out_fs = decim_0_out_fs / 8;
    static constexpr uint32_t final_fs = decim_1_out_fs / 2;  // 24000 Hz

    // Tuning Constants
    static constexpr int32_t LPF_ALPHA_SHIFT = 2;
    static constexpr int32_t MIN_SHIFT_SPREAD = 400;

    // Decimation Chain
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};

    // Demodulator & Audio
    dsp::demodulate::FM demod{};
    AudioOutput audio_output{};

    // Buffers
    std::array<complex16_t, 512> dst_buffer_data{};
    const buffer_c16_t dst_buffer{dst_buffer_data.data(), dst_buffer_data.size()};

    std::array<int16_t, 32> audio_data{};
    const buffer_s16_t audio_buffer{audio_data.data(), audio_data.size()};

    // Output Message
    RTTYDataMessage tx_message{};

    // Demodulator State
    int32_t fm_val_smoothed = 0;

    // Tracker State
    int32_t val_max = 0;
    int32_t val_min = 0;
    uint8_t decay_timer = 0;

    // Auto-Baud State
    uint32_t pulse_measure_counter = 0;
    uint8_t last_bit_state = 0;
    uint32_t estimated_bit_width = 528;  // ~45 baud @ 24k

    // UART State
    enum UartState {
        WAIT_START,
        CHECK_START,
        READ_BITS,
        WAIT_STOP
    };

    UartState uart_state = WAIT_START;
    uint32_t samples_per_bit = 528;
    uint32_t phase_counter = 0;
    uint8_t bit_counter = 0;
    uint8_t shift_reg = 0;
    uint8_t current_slicer_bit = 1;

    // Polarity
    bool inverted_polarity = true;
    uint32_t polarity_timer = 0;

    void configure();
    void process_demodulated_sample(int32_t sample);
    void update_baud_estimation(uint32_t pulse_width);
    void append_data(uint8_t raw_baudot_code);

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_RTTY_RX_H__*/