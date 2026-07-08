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
#include "tonesets.hpp"
#include <algorithm>
#include <cmath>

/**
 * Processor used by epirb_tx app to simulate a COSPAS/SARSAT emergency beacon
 * The processor will alternatively:
 * - Send a 406 MHz Manchester encoded BPSK signal containing the beacon information
 * - Send a 127.5 MHz AM distress audio signal
 */
class EPIRBTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg) override;

   private:
    // True when the processor has received a configuration message from the app
    bool configured{false};
    // True when in 406 MHz transmission mode, false for AM transmission mode
    bool mode_406{false};
    // True when current 406 frame is a 2nd generation DSSS-OQPSK frame
    bool mode_sgb{false};
    // True when in self-test mode (detected from bit 5 of message.data)
    bool mode_sgb_selftest{true};

    // True when the transmission has to be stopped (e.g. at the end of a frame)
    bool end_of_transmission{};
    // I/Q values for current sample (ranging from -128 to 127)
    int8_t re{0}, im{0};

    // Configured pre-count value: used to generate a carrier for config_pre_count samples before starting a frame
    uint32_t config_pre_count = 0;
    // Configured post-count value: used to continue the carrier for config_post_count samples after the end of a frame
    uint32_t config_post_count = 0;

    // Data of the frame to send in 406 mode (FGB BPSK or SGB DSSS-OQPSK)
    static constexpr uint8_t frame_data_max_len = 32;
    static constexpr uint8_t frame_data_fgb_max_len = 18;
    static constexpr uint8_t frame_data_sgb_len = 32;  // ceil(250 / 8)
    uint8_t frame_data[frame_data_max_len]{0};
    // Size of the frame to send in bytes
    uint8_t frame_data_len = 0;
    // Size of the frame payload in bits
    uint16_t frame_sgb_bits_len = 0;

    // BPSK parameters: Target phase +/-1.1 RAD as per COSPAS/SARSAT specifications
    static constexpr float phase_rad = 1.1f;

    // I/Q values for BPSK (positive phase and negative phase)
    int8_t i_pos = (int8_t)(cos(phase_rad) * 127);
    int8_t q_pos = (int8_t)(sin(phase_rad) * 127);
    int8_t i_neg = i_pos;
    int8_t q_neg = -q_pos;
    // I/Q values for carrier only
    static constexpr int8_t i_carrier = 127;
    static constexpr int8_t q_carrier = 0;

    // COSPAS/SARSAT signal is manchester (2 states per bit) encoded 400 bit/sec
    static const uint32_t samples_per_halfbit = TONES_SAMPLERATE / 400 / 2;
    // Sequencer state for BPSK
    uint32_t sample_counter = 0;
    uint32_t bpsk_pre_count = 0;
    uint32_t bpsk_post_count = 0;
    // Bit position in current byte
    uint32_t bit_index = 0;
    // Byte position in current frame
    uint32_t byte_index = 0;
    // Value of the current byte
    uint8_t current_byte = 0;
    // Value of the current bit
    uint8_t current_bit = 0;
    // Position in the current manchester bit
    bool manchester_half = false;  // false = first half

    // 2G SGB DSSS-OQPSK parameters
    static constexpr uint16_t sgb_message_bits = 256; // 250 bits of data + 6 bits padding
    static constexpr uint32_t sgb_chip_rate = 38400;
    static constexpr uint32_t sgb_chips_per_bit = 256;
    static constexpr uint32_t sgb_preamble_bits_per_channel = 25;
    static constexpr uint32_t sgb_bits_per_channel = 150;
    static constexpr uint32_t sgb_segment_chips = 38400;
    static constexpr uint32_t sgb_samples_per_chip = TONES_SAMPLERATE / sgb_chip_rate;
    static constexpr uint32_t sgb_half_chip_samples = sgb_samples_per_chip / 2;
    // SGB burst duration is exactly 1 second.
    static constexpr uint32_t sgb_total_samples = sgb_segment_chips * sgb_samples_per_chip;
    static constexpr int8_t sgb_chip_amplitude = 90;

    static constexpr uint32_t sgb_init_normal_i = 0x1;
    static constexpr uint32_t sgb_init_normal_q = 0x1AC1FC;
    static constexpr uint32_t sgb_init_selftest_i = 0x52C9F0;
    static constexpr uint32_t sgb_init_selftest_q = 0x3CE928;

    struct SGBChannelState {
        uint32_t prn_state = 1;
        uint32_t chip_index = 0;
        uint32_t sample_in_chip = 0;
        int8_t chip_level = 0;
    };

    SGBChannelState sgb_i{};
    SGBChannelState sgb_q{};
    uint32_t sgb_sample_counter = 0;

    uint8_t get_frame_bit(uint16_t bit_pos) const;
    int8_t compute_sgb_chip_level(SGBChannelState& channel, bool q_channel);
    int8_t sample_sgb_channel(SGBChannelState& channel, bool q_channel);
    void init_sgb_channel(SGBChannelState& channel, uint32_t initial_state);

    // 127.5 AM signal parameters
    static const uint32_t sweep_rate = 3;  // 3 Hz
    static const uint32_t f_min = 300;     // Sweep min frequency (Hz)
    static const uint32_t f_max = 1600;    // Sweep max frequency (Hz)
    static const uint32_t freq_span = f_max - f_min;
    // Frequency
    static const uint32_t freq_scale = (1ULL << 32) / TONES_SAMPLERATE;
    static const int32_t center_freq = f_min + (freq_span / 2);
    static const int32_t freq_dev = freq_span / 256;
    // Increments
    static const uint32_t sweep_inc = sweep_rate * freq_scale;

    // Phase accumulators for sweep and audio
    uint32_t sweep_phase = 0;
    uint32_t audio_phase = 0;

    TXProgressMessage txprogress_message{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{TONES_SAMPLERATE, this, baseband::Direction::Transmit};
};

#endif
