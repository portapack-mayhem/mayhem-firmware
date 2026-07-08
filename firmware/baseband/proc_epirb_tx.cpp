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

uint8_t EPIRBTXProcessor::get_frame_bit(uint16_t bit_pos) const {
    // Sckip first 6 bits (padding for 250 bits -> 32 bytes)
    bit_pos += 6;
    if (bit_pos >= frame_sgb_bits_len) {
        return 0;
    }
    const uint8_t byte_value = frame_data[bit_pos >> 3];
    const uint8_t bit_offset = 7 - (bit_pos & 0x07);
    return (byte_value >> bit_offset) & 0x01;
}

int8_t EPIRBTXProcessor::compute_sgb_chip_level(SGBChannelState& channel, bool q_channel) {
    // Table 2.4 behavior: chip = PRN for bit 0, inverted PRN for bit 1.
    // PRN generator (23-bit LFSR): out = reg[0], feedback = reg[0] XOR reg[18].
    const uint8_t prn_chip = channel.prn_state & 0x01;

    uint8_t info_bit = 0;
    const uint32_t bit_in_channel = channel.chip_index / sgb_chips_per_bit;
    if (bit_in_channel >= sgb_preamble_bits_per_channel) {
        const uint32_t message_index = bit_in_channel - sgb_preamble_bits_per_channel;
        if (message_index < (sgb_message_bits / 2)) {
            const uint16_t frame_bit_pos = q_channel ? (message_index * 2) + 1 : (message_index * 2);
            info_bit = get_frame_bit(frame_bit_pos);
        }
    }

    const uint8_t xor_chip = info_bit ^ prn_chip;
    const int8_t chip_level = xor_chip ? -sgb_chip_amplitude : sgb_chip_amplitude;

    const uint8_t feedback = prn_chip ^ ((channel.prn_state >> 18) & 0x01);
    channel.prn_state = (channel.prn_state >> 1) | (uint32_t(feedback) << 22);
    channel.chip_index++;

    return chip_level;
}

int8_t EPIRBTXProcessor::sample_sgb_channel(SGBChannelState& channel, bool q_channel) {
    int8_t level = channel.chip_level;

    if (channel.sample_in_chip == 0) {
        if (channel.chip_index < sgb_segment_chips) {
            channel.chip_level = compute_sgb_chip_level(channel, q_channel);
            level = channel.chip_level;
        } else {
            level = 0;
            channel.chip_level = 0;
        }
    }

    channel.sample_in_chip++;
    if (channel.sample_in_chip >= sgb_samples_per_chip) {
        channel.sample_in_chip = 0;
    }

    return level;
}

void EPIRBTXProcessor::init_sgb_channel(SGBChannelState& channel, uint32_t initial_state) {
    channel.prn_state = initial_state;
    channel.chip_index = 0;
    channel.sample_in_chip = 0;
    channel.chip_level = 0;
}

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

        if (mode_406) {
            if (mode_sgb) {
                // 2G SGB DSSS-OQPSK signal
                const int8_t i_sample = sample_sgb_channel(sgb_i, false);
                const int8_t q_sample = (sgb_sample_counter < sgb_half_chip_samples)
                                            ? 0
                                            : sample_sgb_channel(sgb_q, true);

                re = i_sample;
                im = q_sample;

                sgb_sample_counter++;
                if (sgb_sample_counter >= sgb_total_samples) {
                    sgb_sample_counter = 0;
                    end_of_transmission = true;
                }
            } else {
                // 1G BPSK Manchester beacon signal
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
            mode_406 = message.mode_406;
            if (mode_406) {
                // 406 MHz frame mode:
                // - FGG if data_len <= 144 bits (i.e. 18 bytes)
                // - SGB if data_len > 144 bits (actually 250 bits (i.e. 32 bytes))
                config_pre_count = message.pre_count;
                config_post_count = message.post_count;

                mode_sgb = message.data_len > frame_data_fgb_max_len;  // SGB if data_len > 18 bytes (144 bits)
                frame_data_len = message.data_len;

                if (mode_sgb) {
                    frame_sgb_bits_len = std::min<uint16_t>(((uint16_t)message.data_len) * 8, sgb_message_bits);  // Total bits in the frame
                    frame_data_len = std::min<uint8_t>(frame_data_len, frame_data_sgb_len);
                    // Detect self-test mode: bit 5 of message.data[0]
                    mode_sgb_selftest = (message.data[0] >> 3) & 0x01;
                } else {
                    frame_data_len = std::min<uint8_t>(frame_data_len, frame_data_fgb_max_len);
                }

                // Get the frame data from the message
                memcpy(frame_data, message.data, frame_data_len);

                if (mode_sgb) {
                    // Init SGB DSSS-OQPSK sequencer
                    sgb_sample_counter = 0;
                    const uint32_t init_i = mode_sgb_selftest ? sgb_init_selftest_i : sgb_init_normal_i;
                    const uint32_t init_q = mode_sgb_selftest ? sgb_init_selftest_q : sgb_init_normal_q;
                    init_sgb_channel(sgb_i, init_i);
                    init_sgb_channel(sgb_q, init_q);
                } else {
                    // Init FGB BPSK sequencer
                    sample_counter = 0;
                    bpsk_pre_count = 0;
                    bpsk_post_count = 0;
                    bit_index = 0;
                    byte_index = 0;
                    current_byte = 0;
                    current_bit = 0;
                }
            } else {
                // AM mode for 121.5 signal => init AM sequencer
                sweep_phase = 0;
                audio_phase = 0;
                mode_sgb = false;
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
