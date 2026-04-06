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

#include "proc_epirb_tx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>
#include <cstring>

/**
 * Processing method for this processor
 */
void EPIRBTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Iterate on each sample of the buffer
    for (size_t i = 0; i < buffer.count; i++) {
        if (end_of_transmission) {
            // Stop transmission
            configured = false;
            end_of_transmission = false;
            txprogress_message.done = true;
            shared_memory.application_queue.push(txprogress_message);
        }

        if (mode_bpsk) {
            // BPSK Manchester beacon signal
            if (bpsk_pre_count < config_pre_count) {
                // Pre-count state: send carrier only during pre-count
                bpsk_pre_count++;
                re = i_carrier;
                im = q_carrier;
            } else if (bpsk_post_count > 0) {
                // Post-count: send carrier only during post-count
                bpsk_post_count++;
                re = i_carrier;
                im = q_carrier;
                if (bpsk_post_count >= config_post_count) {
                    // End transmission here
                    byte_index = 0;
                    bpsk_post_count = 0;
                    bpsk_pre_count = 0;
                    end_of_transmission = true;
                }
            } else {
                if (sample_counter == 0 && manchester_half == false) {
                    if (bit_index == 0) {
                        // Read current byte
                        current_byte = frame_data[byte_index];
                        // Move to next byte
                        byte_index++;
                    }
                    // Get current bit
                    current_bit = (current_byte >> (7 - bit_index)) & 0x01;
                }

                // Manchester encoding
                if (current_bit == 1) {
                    // 1 = falling signal
                    if (manchester_half == false) {
                        re = i_pos;
                        im = q_pos;
                    } else {
                        re = i_neg;
                        im = q_neg;
                    }
                } else {
                    // 0 = rising signal
                    if (manchester_half == false) {
                        re = i_neg;
                        im = q_neg;
                    } else {
                        re = i_pos;
                        im = q_pos;
                    }
                }
                // Move to next sample
                sample_counter++;

                if (sample_counter >= samples_per_halfbit) {
                    // Move to next half-bit
                    sample_counter = 0;
                    manchester_half = !manchester_half;

                    // Next bit after two half bits
                    if (manchester_half == false) {
                        // Move to next bit
                        bit_index++;
                        if (bit_index >= 8) {
                            // End of byte
                            bit_index = 0;
                            if (byte_index >= frame_data_len) {
                                // End of frame => move to post-count
                                bpsk_post_count = 1;
                            }
                        }
                    }
                }
            }
        } else {
            // AM 127.5 MHz sine sweep
            // ---- 3 Hz Sweep ----
            sweep_phase += sweep_inc;
            uint8_t sweep_index = (sweep_phase & 0xFF000000) >> 24;
            int8_t sweep = sine_table_i8[sweep_index];  // -128..127

            // Audio frequency based on sweep
            int32_t audio_freq = center_freq + sweep * freq_dev;

            // ---- Audio signal (sine wave) ----
            uint32_t audio_inc = audio_freq * freq_scale;
            audio_phase += audio_inc;

            uint8_t audio_index = (audio_phase & 0xFF000000) >> 24;
            int8_t audio = sine_table_i8[audio_index];

            // ---- AM ----
            // Double Side Band modulation with modulation index of ~80% (100/128) + offset (74)
            int16_t amplitude = 74 + ((100 * audio) >> 7);  // 1/128 via shift

            if (amplitude > 127) amplitude = 127;
            if (amplitude < -128) amplitude = -128;

            re = (int8_t)amplitude;
            im = 0;
        }
        buffer.p[i] = {re, im};
    }
};

void EPIRBTXProcessor::on_message(const Message* const msg) {
    // Configure the processor
    switch (msg->id) {
        case Message::ID::EPIRBTXData: {
            const auto message = *reinterpret_cast<const EPIRBTXDataMessage*>(msg);
            // Check transmission mode
            mode_bpsk = message.mode_bpsk;
            if (mode_bpsk) {
                // BPSK mode for 406 frame
                config_pre_count = message.pre_count;
                config_post_count = message.post_count;
                frame_data_len = message.data_len;
                // Get the frame data from the message
                memcpy(frame_data, message.data, std::min(frame_data_len, EPIRBTXDataMessage::max_len));
                // Init BPSK sequencer
                sample_counter = 0;
                bpsk_pre_count = 0;
                bpsk_post_count = 0;
                bit_index = 0;
                byte_index = 0;
                current_byte = 0;
                current_bit = 0;
            } else {
                // AM mode for 121.5 signal => init AM sequencer
                sweep_phase = 0;
                audio_phase = 0;
            }
            // Tell the processor to start
            configured = true;
        } break;

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<EPIRBTXProcessor>()};
    event_dispatcher.run();
    return 0;
}
