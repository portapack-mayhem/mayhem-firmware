/*
 * Copyright (C) 2024 EPIRB Receiver Implementation
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

#include "proc_epirb.hpp"

#include "portapack_shared_memory.hpp"

#include "dsp_fir_taps.hpp"

#include "audio_dma.hpp"

#include "event_m4.hpp"
#include <ch.h>

#define CONF_FLOAT(F) (uint32_t)(fabsf(F) * 100)

EPIRBProcessor::EPIRBProcessor() {
    // Configure the decimation filters for narrowband EPIRB signal
    // Target: Reduce 2.457600 MHz to ~38.4 kHz for 800 bps processing
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);
    demod.configure(SAMPLE_RATE, 5000);
    configure_audio();
    channel_spectrum.set_decimation_factor(1);
    baseband_thread.start();
}

void EPIRBProcessor::configure_audio() {
    float squelch_threshold = ((float)squelch_level)/100.0f;
    audio_output.configure(audio_24k_hpf_300hz_config, audio_24k_deemph_300_6_config, squelch_threshold);
}

float EPIRBProcessor::get_phase_diff(const complex16_t& sample0, const complex16_t& sample1) {
    // Calculate the phase difference between two samples.
    float dI = sample1.real() * sample0.real() + sample1.imag() * sample0.imag();
    float dQ = sample1.imag() * sample0.real() - sample1.real() * sample0.imag();
    float phase_diff = atan2f(dQ, dI);
    if (phase_diff > M_PI) phase_diff -= 2.0f * M_PI;
    if (phase_diff < -M_PI) phase_diff += 2.0f * M_PI;
    return phase_diff;
}

bool EPIRBProcessor::filtered_rise_detect(bool condition) {
    bool result = false;
    if (condition) {
        rise_detection_count++;
        if (rise_detection_count >= RISE_FILTER_SAMPLES) {
            result = true;
            rise_detection_count = 0;
        }
    } else {
        rise_detection_count = 0;
    }
    return result;
}

void EPIRBProcessor::execute(const buffer_c8_t& buffer) {
    // First decimation stage: 3.072000 MHz / 8 -> 384 kHz
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);

    // Second decimation stage: 384 kHz / 8 -> 48 kHz
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    // We use decim1 output as decimator output
    const auto decimator_out = decim_1_out;

    // Feed IQ data into spectrum collector for the RF waterfall.
    if(spectrum_on) channel_spectrum.feed(decim_1_out, -5500, 5500, 3400);

    feed_channel_stats(decimator_out);

    if(audio_on) {
        // Channel filter for audio out
        const auto  channel_out = channel_filter.execute(decim_1_out, dst_buffer);
        auto audio = demod.execute(channel_out, audio_buffer);
        // auto audio = demod.execute(decimator_out, audio_buffer);
        audio_output.write(audio);
    }

    // Process each decimated sample through the matched filter
    for (size_t i = 0; i < decimator_out.count; i++) {
        // Track sample count since last symbol and since begining of the frame
        sample_count++;
        frame_sample_count++;
        // Compute phase delta since last sample
        float phase_delta = get_phase_diff(last_sample, decimator_out.p[i]);
        last_sample = decimator_out.p[i];

        // Let's sum phase delta over a 6 sample window to get the full phase jump
        phase_delta_acc -= phase_delta_buffer[pahse_delta_index];
        phase_delta_buffer[pahse_delta_index] = phase_delta;
        phase_delta_acc += phase_delta_buffer[pahse_delta_index];
        pahse_delta_index = (pahse_delta_index + 1) % PHASE_DELTA_ACC_SIZE;

        // Use accumulated delta
        phase_delta = phase_delta_acc;

        // State machine for COSPAS frame detection
        switch (current_state) {
            case IDLE:
                // We are waiting for a 160ms empty carrier => phase shouls be stable during this period
                if (filtered_rise_detect(phase_delta >= 0.6f)) {
                    stability_counter = 0;
                } else {
                    stability_counter++;
                    if (stability_counter > CARRIER_SAMPLES_THRESHOLD) {
                        //send_packet(0xFED0000000000001);
                        current_state = CARRIER_LOCKED;
                        frame_sample_count = 0;
                    }
                }
                break;

            case CARRIER_LOCKED:
                // Carrier is locked, we now wait for a phase 1.1 rad phase jump corresponding to the befining of the frame

                if (filtered_rise_detect(phase_delta >= 0.7f)) {
                    // Jump detected (1.1 rad)
                    frame_sample_count = 0;
                    // send_packet(0xFED0000000000002);
                    //send_packet(0xFEE0000000000000 | CONF_FLOAT(phase_delta));
                    current_state = DATA_SYNC;
                    // send_packet(0xFEA0000000000000 | CONF_FLOAT(avg_phase));
                    // send_packet((((phase - avg_phase)>=0) ? 0xFEA0000000000000 : 0xFEB0000000000000) | CONF_FLOAT(phase));
                    last_phase_positive = true;
                    last_bit = true;
                } else if (frame_sample_count > CARRIER_MAX_SAMPLES) {
                    frame_end();
                }
                break;

            case DATA_SYNC: {
                float abs_phase_delta = fabsf(phase_delta);

                if (abs_phase_delta >= 1.6f) {
                    // Phase should jump from 1.1 rad to -1.1 rad or the other way around
                    // Absolute phase jump is expected to be 2.2 rad
                    // Phase jump is either positive or negative
                    bool phase_positive = (phase_delta >= 0.0f);
                    // send_packet((phase_positive ? 0xFCB0000000000000 : 0xFCC0000000000000) | CONF_FLOAT(phase_delta));

                    if (phase_positive != last_phase_positive) {
                        // Phase jumped to the opposit direction of last jump
                        // send_packet((phase_positive ? 0xFEB0000000000000 : 0xFEC0000000000000) | CONF_FLOAT(phase_delta));
                        // send_packet((phase_positive ? 0xFAB0000000000000 : 0xFAC0000000000000) | frame_sample_count);
                        last_phase_positive = phase_positive;
                        bool cur_bit;
                        // Phase change => how long since last change ?
                        if ((frame_sample_count >= (SAMPLES_PER_SYMBOL - SAMPLES_MARGIN)) && (frame_sample_count <= (SAMPLES_PER_SYMBOL + SAMPLES_MARGIN))) {
                            // Frame start
                            if (!phase_positive) {
                                // Search for falling edge
                                // send_packet(0xFAB000000000000B);
                                cur_bit = true;
                            } else {
                                continue;
                            }
                        } else if (sample_count > (SAMPLES_PER_SYMBOL * 2 + SAMPLES_MARGIN)) {
                            // We missed something...
                            //send_packet(0xFED0000000000003);
                            //send_packet(0xFEA0000000000000 | (uint32_t)sample_count);
                            // TODO
                            cur_bit = last_bit;
                        } else if (sample_count >= (SAMPLES_PER_SYMBOL * 2 - SAMPLES_MARGIN)) {
                            // 2 symbols since last change => bit value changes
                            cur_bit = !last_bit;
                        } else if ((sample_count >= (SAMPLES_PER_SYMBOL - SAMPLES_MARGIN)) && (sample_count <= (SAMPLES_PER_SYMBOL + SAMPLES_MARGIN))) {
                            // Phase change occured in first half bit => we keep the same value
                            if ((phase_positive && last_bit) || (!phase_positive && !last_bit)) {
                                sample_count = 0;
                                // Ignore rising edge if current value is 1 and falling edge if current value is 0
                                continue;
                            }
                            // Same value on falling/rising edge
                            cur_bit = last_bit;
                        } else {
                            // Filter the rest
                            continue;
                        }
                        sample_count = 0;
                        packet_builder.execute(cur_bit);
                        last_bit = cur_bit;
                        /*bit_history.add(cur_bit);
                        history_size++;
                        if (history_size >= 64) {
                            history_size = 0;
                            // Create and send EPIRB packet message to application layer
                            send_packet(bit_history.value());
                            bit_history = BitHistory();
                        }*/
                    }
                }
                if (frame_sample_count > FRAME_MAX_SAMPLES) {
                    current_state = POST_FRAME;
                    packet_builder.flush();
                }
            } break;
            case POST_FRAME:
                if (frame_sample_count > CARRIER_MAX_SAMPLES) {
                    frame_end();
                }
            default:
                break;
        }
    }
}

void EPIRBProcessor::frame_end() {
    // uint16_t old_count = frame_sample_count;
    sample_count = 0;
    frame_sample_count = 0;
    stability_counter = 0;
    last_phase_positive = false;
    last_bit = false;
    current_state = IDLE;
    // send_packet(0xFED0000000000000);
    // send_packet(0xFEA0000000000000 | old_count);
    /*if (history_size > 0) send_packet(bit_history.value());
    history_size = 0;
    bit_history = BitHistory();*/
    // Reset packet builder
    packet_builder.reset_state();
}

/*
void EPIRBProcessor::send_packet(uint64_t data) {
    baseband::Packet packet{};
    for (int8_t i = 63; i >= 0; i--) {
        packet.add((data >> i) & 0x1);
    }
    const EPIRBPacketMessage message{packet};
    shared_memory.application_queue.push(message);
}
*/

void EPIRBProcessor::payload_handler(const baseband::Packet& packet) {
    // EPIRB packet received - validate and process
    // send_packet(0xFEC0000000000000 | frame_sample_count);
    //  Create and send EPIRB packet message to application layer
    const EPIRBPacketMessage message{packet};
    shared_memory.application_queue.push(message);
}

void EPIRBProcessor::on_message(const Message* const msg) {
    // Configure the processor
    switch (msg->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(msg);
            break;
        case Message::ID::EPIRBRXConfig: {
            const EPIRBRXConfig message = *reinterpret_cast<const EPIRBRXConfig*>(msg);
            audio_on = message.audio_on;
            spectrum_on = message.scpectrum_on;
            if(message.squelch != squelch_level) {
                squelch_level = message.squelch;
                configure_audio();
            }
        } break;

        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<EPIRBProcessor>()};
    event_dispatcher.run();
    return 0;
}