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

void EPIRBTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    for (size_t i = 0; i < buffer.count; i++) {
        if (end_of_transmission) {
            // Stop transmission
            configured = false;
            end_of_transmission = false;
            txprogress_message.done = true;
            shared_memory.application_queue.push(txprogress_message);
        }

        if(mode_bpsk)
        {
            // BPSK Manchester
            if (bpsk_pre_count < config_pre_count) {
                bpsk_pre_count++;
                re = i_neg;
                im = q_neg;
            } else if (bpsk_post_count > 0) {
                bpsk_post_count++;
                re = i_neg;
                im = q_neg;
                if (bpsk_post_count >= config_post_count) {  // End of transmission
                    byte_index = 0;                      // Stop here
                    bpsk_post_count = 0;
                    bpsk_pre_count = 0;
                    end_of_transmission = true;
                }
            } else {
                if (sample_counter == 0 && manchester_half == false) {
                    if (bit_index == 0) {
                        if (byte_index >= frame_data_len) {  // End of frame => move to postcount
                            bpsk_post_count = 1;
                        }
                        current_byte = frame_data[byte_index];
                        byte_index ++;
                    }

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

                sample_counter++;

                if (sample_counter >= samples_per_halfbit) {
                    sample_counter = 0;
                    manchester_half = !manchester_half;

                    // Next bit aftoer to half bits
                    if (manchester_half == false) {
                        bit_index++;
                        if (bit_index >= 8)
                            bit_index = 0;
                    }
                }
            }
        }
        else
        {   // AM 127.5 MHz sine swee
            // ---- 2 Hz Sweep ----
            sweep_phase += sweep_inc;
            uint8_t sweep_index = (sweep_phase & 0xFF000000) >> 24;
            int8_t sweep = sine_table_i8[sweep_index];   // -128..127

            // Fréquence instantanée
            int32_t audio_freq = center_freq + sweep * freq_dev;

            // ---- Audio ----
            uint32_t audio_inc = audio_freq * freq_scale;
            audio_phase += audio_inc;

            uint8_t audio_index = (audio_phase & 0xFF000000) >> 24;
            int8_t audio = sine_table_i8[audio_index];

            // ---- AM ----
            // int16_t amplitude = 64 + (audio >> 1);  // 64 = 127 - (127 >> 1): carrier level without modulating signal //127 + ((modulation_index * audio) >> 7);   // /128 via shift
            int16_t amplitude = 74 + ((100 * audio) >> 7);   // /128 via shift

            if (amplitude > 127) amplitude = 127;
            if (amplitude < -128) amplitude = -128;

            re = (int8_t)amplitude;
            im = 0;
        }
        buffer.p[i] = {re, im};
    }
};

void EPIRBTXProcessor::on_message(const Message* const msg) {
    const auto message = *reinterpret_cast<const EPIRBTXDataMessage*>(msg);

    switch (msg->id) {
        case Message::ID::EPIRBTXData :
            mode_bpsk = message.mode_bpsk;
            if(mode_bpsk)
            {   // BPSK mode for 406 frame
                config_pre_count = message.pre_count;
                config_post_count = message.post_count;
                frame_data_len = message.data_len;
                memcpy(frame_data,message.data,frame_data_len);
                // Init BPSK
                sample_counter = 0;
                bpsk_pre_count = 0;
                bpsk_post_count = 0;
                bit_index = 0;
                byte_index = 0;
                current_byte = 0;
                current_bit = 0;
            }
            else
            {   // AM mode for 121.5 signal
                sweep_phase = 0;
                audio_phase = 0;  
            }
            configured = true;
            break;

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<EPIRBTXProcessor>()};
    event_dispatcher.run();
    return 0;
}
