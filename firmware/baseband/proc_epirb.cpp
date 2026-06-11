/*
 * Copyright (C) 2024 EPIRB Receiver Implementation
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

#include "proc_epirb.hpp"

#include "portapack_shared_memory.hpp"

#include "dsp_fir_taps.hpp"

#include "audio_dma.hpp"

#include "event_m4.hpp"

#include <algorithm>
#include <cmath>

#include <ch.h>
EPIRBProcessor::EPIRBProcessor() {
    // Configure the decimation filters for narrowband EPIRB signal
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    // Configure channel filter for audio filtering
    channel_filter.configure(taps_11k0_channel.taps, 2);
    // Configure demodulation for audio output
    demod.configure(SAMPLE_RATE, 5000);
    // Configure audio output (+squelch level)
    configure_audio();
#ifdef SPECAN
    channel_spectrum.set_decimation_factor(1);
#endif
    baseband_thread.start();
}

void EPIRBProcessor::configure_audio() {
    // UI sends an squelch value ranging from 0 to 99, 0 disables squelch, dividing UI value by 40 gives a valid UI threashold around 50
    audio_output.configure(audio_24k_hpf_300hz_config, audio_24k_deemph_300_6_config, ((float)squelch_level) / 40.0f);
}

float EPIRBProcessor::get_phase_diff(const complex16_t& sample0, const complex16_t& sample1) {
    // Calculate the phase difference between two samples
    float dI = sample1.real() * sample0.real() + sample1.imag() * sample0.imag();
    float dQ = sample1.imag() * sample0.real() - sample1.real() * sample0.imag();
    float phase_diff = atan2f(dQ, dI);
    return phase_diff;
}

bool EPIRBProcessor::filtered_rise_detect(bool condition) {
    bool result = false;
    if (condition) {
        // If rise condition is matched, filter peaks that last less than 3 samples
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

#ifdef SPECAN
    // Feed IQ data into spectrum collector for the RF waterfall.
    if (spectrum_on) channel_spectrum.feed(decim_1_out, -5500, 5500, 3400);
#endif

    feed_channel_stats(decimator_out);

    // if (audio_on) {
    //  Channel filter for audio out
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    auto audio = demod.execute(channel_out, audio_buffer);
    audio_output.write(audio);
    //}

    // Process each decimated sample through state machine
    for (size_t i = 0; i < decimator_out.count; i++) {
        // Track sample count since last symbol and since begining of the frame
        sample_count++;
        frame_sample_count++;
        // Compute phase delta since last sample
        float phase_delta = get_phase_diff(last_sample, decimator_out.p[i]);
        last_sample = decimator_out.p[i];

        // AFC: remove the estimated carrier frequency offset from the raw delta
        // before any further processing. Done on the per-sample value so the
        // 12-sample accumulator below tracks it naturally.
        phase_delta -= freq_offset_est;

        // Keep the (de-biased) per-sample delta for AFC averaging over the carrier.
        const float sample_phase_delta = phase_delta;

        // Let's sum phase delta over a 12 sample window to get the full phase jump
        phase_delta_acc -= phase_delta_buffer[phase_delta_index];
        phase_delta_buffer[phase_delta_index] = phase_delta;
        phase_delta_acc += phase_delta_buffer[phase_delta_index];
        phase_delta_index = (phase_delta_index + 1) % PHASE_DELTA_ACC_SIZE;

        // Use accumulated delta
        phase_delta = phase_delta_acc;

        // State machine for COSPAS frame detection
        switch (current_state) {
            case IDLE: {
                // Continuously pull the AFC estimate toward the mean per-sample
                // rotation so the accumulator self-centers for any offset up to
                // the discriminator Nyquist (~+/-24 kHz). On noise the de-biased
                // deltas average to ~0, so the estimate stays put; on a real
                // carrier it converges within a few ms and the thresholds below
                // then see a de-biased signal regardless of the actual offset.
                // Only update AFC when the per-sample phase delta is small
                // (large jumps indicate noise or transient, which would cause
                // a random-walk drift if used for AFC updates).
                if (fabsf(sample_phase_delta) <= AFC_UPDATE_PHASE_MAX) {
                    freq_offset_est += AFC_TRACK_ALPHA * sample_phase_delta;
                    // Bounds checking: limit to ±5 kHz (~0.654 rad/sample at 48 kHz)
                    freq_offset_est = std::clamp(freq_offset_est, -0.654f, 0.654f);

                    // Track AFC convergence using Welford's online algorithm
                    afc_convergence_n++;
                    float delta = freq_offset_est - afc_mean;
                    afc_mean += delta / afc_convergence_n;
                    float delta2 = freq_offset_est - afc_mean;
                    afc_m2 += delta * delta2;
                }

                // We are waiting for a 160ms empty carrier => phase should be stable during this period
                // Use a symmetric threshold: once AFC has removed the bias a stable
                // carrier sits near 0, so both positive and negative excursions of
                // the accumulated delta indicate the carrier is not yet stable.
                if (filtered_rise_detect(fabsf(phase_delta) >= 0.6f)) {
                    stability_counter = 0;
                    // Reset convergence tracking when the carrier is not stable,
                    // so variance is measured only over the current stable window.
                    afc_mean = 0.0f;
                    afc_m2 = 0.0f;
                    afc_convergence_n = 0;
                } else {
                    stability_counter++;
                    // Check both phase stability AND AFC convergence before transitioning
                    if (stability_counter > CARRIER_SAMPLES_THRESHOLD) {
                        float afc_variance = (afc_convergence_n > 1) ? afc_m2 / (afc_convergence_n - 1) : 0.0f;
                        if (afc_variance < AFC_CONVERGENCE_THRESHOLD) {
                            // Both phase and AFC have converged, go to locked state
                            current_state = CARRIER_LOCKED;
                            // Reset carrier accumulators so the latched update uses
                            // only the residual measured while in the locked window
                            carrier_phase_sum = 0.0f;
                            carrier_phase_n = 0;
                            frame_sample_count = 0;
                        }
                    }
                }
            } break;

            case CARRIER_LOCKED:
                // Carrier is locked: this is the clean unmodulated carrier window.
                // Average the per-sample phase delta here to estimate the residual
                // frequency offset (rad/sample) used for AFC.
                carrier_phase_sum += sample_phase_delta;
                carrier_phase_n++;
                // Carrier is locked, we now wait for a phase 1.1 rad phase jump corresponding to the beginning of the frame
                // Let's use a 0.7 phase jump threshold
                if (filtered_rise_detect(phase_delta >= 0.7f)) {
                    // Latch the AFC estimate from the carrier we just measured so it
                    // applies to the data burst that starts now. Accumulate so the
                    // residual is folded into any prior estimate.
                    if (carrier_phase_n > 0) {
                        freq_offset_est += carrier_phase_sum / carrier_phase_n;
                        // Bounds checking: limit to ±5 kHz (~0.654 rad/sample at 48 kHz)
                        freq_offset_est = std::clamp(freq_offset_est, -0.654f, 0.654f);
                    }
                    // Jump detected, frame starts now
                    frame_sample_count = 0;
                    // Go to data sync state
                    current_state = DATA_SYNC;
                    // Frame should always start with a positive phase shift
                    last_phase_positive = true;
                    // And a 1 value
                    last_bit = true;
                } else if (frame_sample_count > CARRIER_MAX_SAMPLES) {
                    // We missed sync pattern
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

                    if (phase_positive != last_phase_positive) {
                        // Phase jumped to the opposite direction of last jump
                        last_phase_positive = phase_positive;
                        bool cur_bit;
                        // Phase change => how long since last change ?
                        if ((frame_sample_count >= (SAMPLES_PER_SYMBOL - SAMPLES_MARGIN)) && (frame_sample_count <= (SAMPLES_PER_SYMBOL + SAMPLES_MARGIN))) {
                            // Frame start
                            if (!phase_positive) {
                                // Symbol detection is made on falling edge
                                cur_bit = true;
                            } else {
                                // Ignore rising edge
                                continue;
                            }
                        } else if (sample_count > (SAMPLES_PER_SYMBOL * 2 + SAMPLES_MARGIN)) {
                            // We missed something...
                            // Let's keep same value for current bit
                            cur_bit = last_bit;
                        } else if (sample_count >= (SAMPLES_PER_SYMBOL * 2 - SAMPLES_MARGIN)) {
                            // 2 symbols since last change => bit value changes
                            cur_bit = !last_bit;
                        } else if ((sample_count >= (SAMPLES_PER_SYMBOL - SAMPLES_MARGIN)) && (sample_count <= (SAMPLES_PER_SYMBOL + SAMPLES_MARGIN))) {
                            // Phase change occurred in first half bit => we keep the same value
                            if ((phase_positive && last_bit) || (!phase_positive && !last_bit)) {
                                sample_count = 0;
                                // Ignore rising edge if current value is 1 and falling edge if current value is 0 and move to next symbol
                                continue;
                            }
                            // Same value on falling/rising edge
                            cur_bit = last_bit;
                        } else {
                            // Filter the rest
                            continue;
                        }
                        // Store new bit and move to next symbol
                        sample_count = 0;
                        packet_builder.execute(cur_bit);
                        last_bit = cur_bit;
                    }
                }
                if (frame_sample_count > FRAME_MAX_SAMPLES) {
                    // End of frame
                    current_state = POST_FRAME;
                    packet_builder.flush();
                }
            } break;
            case POST_FRAME:
                if (frame_sample_count > CARRIER_MAX_SAMPLES) {
                    // End of carrier
                    frame_end();
                }
            default:
                break;
        }
    }
}

void EPIRBProcessor::frame_end() {
    sample_count = 0;
    frame_sample_count = 0;
    stability_counter = 0;
    last_phase_positive = false;
    last_bit = false;
    current_state = IDLE;
    // Reset AFC so the next burst is re-estimated from its own carrier preamble.
    freq_offset_est = 0.0f;
    carrier_phase_sum = 0.0f;
    carrier_phase_n = 0;
    // Reset AFC convergence tracking for next frame
    afc_mean = 0.0f;
    afc_m2 = 0.0f;
    afc_convergence_n = 0;
    packet_builder.reset_state();
}

void EPIRBProcessor::payload_handler(const baseband::Packet& packet) {
    // EPIRB packet received: create and send EPIRB packet message to application layer
    const EPIRBPacketMessage message{packet};
    shared_memory.application_queue.push(message);
}

void EPIRBProcessor::on_message(const Message* const msg) {
    // Configure the processor
    switch (msg->id) {
#ifdef SPECAN
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(msg);
            break;
#endif
        case Message::ID::EPIRBRXConfig: {
            const EPIRBRXConfig message = *reinterpret_cast<const EPIRBRXConfig*>(msg);
            // audio_on = message.audio_on;
#ifdef SPECAN
            spectrum_on = message.spectrum_on;
#endif
            if (message.squelch != squelch_level) {
                // Update squelch config
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
