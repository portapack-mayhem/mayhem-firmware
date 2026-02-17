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

#include "proc_rtty_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_dma.hpp"
#include "event_m4.hpp"

// RTTY Timing Limits (at 24kHz)
static constexpr uint32_t MIN_VALID_PULSE = 200;
static constexpr uint32_t MAX_VALID_PULSE = 800;

void RTTYRxProcessor::configure() {
    configured = false;

    baseband_thread.set_sampling_rate(baseband_fs);

    // 1. 3.072M -> 384k
    decim_0.configure(taps_4k25_decim_0.taps);

    // 2. 384k -> 48k
    decim_1.configure(taps_4k25_decim_1.taps);

    // 3. 48k -> 24k
    channel_filter.configure(taps_11k0_channel.taps, 2);

    // FM Demodulator
    demod.configure(24000, 9000);

    // Audio Output
    audio_output.configure(iir_config_passthrough, iir_config_passthrough, 1.0f);

    // Reset State
    val_max = -200000;
    val_min = 200000;

    uart_state = WAIT_START;

    inverted_polarity = false;
    // Default to standard 45.45 baud
    estimated_bit_width = 528;
    samples_per_bit = 528;
    pulse_measure_counter = 0;

    configured = true;
}

// Variables for Fast-Lock Auto Baud
uint32_t candidate_width = 0;
uint8_t candidate_hits = 0;
uint32_t squelch_closed_timer = 0;
bool is_squelched = true;

void RTTYRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    buffer_c16_t decim_1_target{dst_buffer_data.data() + 256, 256};
    const auto decim_1_out = decim_1.execute(decim_0_out, decim_1_target);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

    auto audio = demod.execute(channel_out, audio_buffer);

    feed_channel_stats(channel_out);

    for (size_t i = 0; i < audio.count; i++) {
        int16_t sample = audio.p[i];

        // DECODER INPUT
        int32_t fm_val = (int32_t)sample * 32;

        // 1. ENVELOPE TRACKING (Floating)
        if (fm_val > val_max) val_max = fm_val;
        if (fm_val < val_min) val_min = fm_val;

        // 2. DECAY
        // Shrink the envelope spread (Max - Min) slowly.
        if (++decay_timer == 0) {
            int32_t spread = val_max - val_min;
            if (spread > 200) {
                int32_t decay = (spread >> 7) + 1;
                if (val_max > val_min + decay) val_max -= decay;
                if (val_min < val_max - decay) val_min += decay;
            } else {
                // val_max += 100;
                // val_min -= 100;
                // temp off. but kept it for memory
            }
        }

        // 3. OFFSET CALCULATION
        int32_t midpoint = (val_max + val_min) / 2;

        // 4. SIGNAL CENTERING
        int32_t centered_val = fm_val - midpoint;

        // LPF for Slicer
        fm_val_smoothed += (centered_val - fm_val_smoothed) >> LPF_ALPHA_SHIFT;

        // 5. SQUELCH
        int32_t spread = val_max - val_min;

        if (is_squelched) {
            if (spread > 600) is_squelched = false;
        } else {
            if (spread < 300) is_squelched = true;
        }

        process_demodulated_sample(fm_val_smoothed);

        // 6. AUDIO PATH
        if (is_squelched) {
            audio.p[i] = 0;
        } else {
            int32_t audio_boost = sample * 48;
            if (audio_boost > 32767)
                audio_boost = 32767;
            else if (audio_boost < -32768)
                audio_boost = -32768;
            audio.p[i] = (int16_t)audio_boost;
        }
    }

    audio_output.write(audio);

    // UI Update
    if (tx_message.data_len > 0) {
        if (baud_rate == 0 && samples_per_bit > 0) {
            uint32_t b = (final_fs * 100) / samples_per_bit;
            if (b > 4300 && b < 4700)
                b = 4500;
            else if (b > 4800 && b < 5200)
                b = 5000;
            else if (b > 7200 && b < 7800)
                b = 7500;
            tx_message.baud = (uint16_t)b;
        } else {
            tx_message.baud = baud_rate;
        }
        tx_message.shift = shift_hz;

        if (shared_memory.application_queue.push(tx_message)) {
            tx_message.data_len = 0;
        }
    }
}

void RTTYRxProcessor::process_demodulated_sample(int32_t sample) {
    // 1. Squelch Check
    if (is_squelched) {
        squelch_closed_timer++;
        if (squelch_closed_timer > 12000) {
            uart_state = WAIT_START;
            current_slicer_bit = 1;
            pulse_measure_counter = 0;
            inverted_polarity = false;
            if (baud_rate == 0) {
                estimated_bit_width = 528;
                samples_per_bit = 528;
            }
        }
        return;
    }

    squelch_closed_timer = 0;

    // 2. Schmitt Trigger
    int32_t hysteresis = (val_max - val_min) / 8;

    uint8_t raw_bit = current_slicer_bit;
    if (inverted_polarity) raw_bit = !raw_bit;

    if (sample > hysteresis)
        raw_bit = 1;
    else if (sample < -hysteresis)
        raw_bit = 0;

    // Polarity Check
    if (raw_bit == 0) {
        if (++polarity_timer > 7200) {
            inverted_polarity = !inverted_polarity;
            polarity_timer = 0;
            val_max = -200000;
            val_min = 200000;
            uart_state = WAIT_START;
        }
    } else {
        polarity_timer = 0;
    }

    current_slicer_bit = inverted_polarity ? !raw_bit : raw_bit;

    // 3. Auto Baud
    if (baud_rate == 0) {
        pulse_measure_counter++;
        if (current_slicer_bit != last_bit_state) {
            update_baud_estimation(pulse_measure_counter);
            pulse_measure_counter = 0;
            last_bit_state = current_slicer_bit;
        }
    }

    // 4. UART State Machine
    switch (uart_state) {
        case WAIT_START:
            if (current_slicer_bit == 0) {
                phase_counter = samples_per_bit / 2;
                uart_state = CHECK_START;
            }
            break;

        case CHECK_START:
            if (--phase_counter == 0) {
                if (current_slicer_bit == 0) {
                    phase_counter = samples_per_bit;
                    bit_counter = 0;
                    shift_reg = 0;
                    uart_state = READ_BITS;
                } else {
                    uart_state = WAIT_START;
                }
            }
            break;

        case READ_BITS:
            if (--phase_counter == 0) {
                if (current_slicer_bit) shift_reg |= (1 << bit_counter);

                phase_counter = samples_per_bit;
                bit_counter++;

                if (bit_counter >= 5) {
                    uart_state = WAIT_STOP;
                }
            }
            break;

        case WAIT_STOP:
            if (--phase_counter == 0) {
                // Accept data even if stop bit is noisy (0)
                // This improves reception during fades
                // if (current_slicer_bit == 1) {
                append_data(shift_reg & 0x1F);
                //}
                uart_state = WAIT_START;
            }
            break;
    }
}

void RTTYRxProcessor::update_baud_estimation(uint32_t pulse_width) {
    if (pulse_width < MIN_VALID_PULSE || pulse_width > MAX_VALID_PULSE) return;

    int32_t diff = (int32_t)pulse_width - (int32_t)estimated_bit_width;
    if (diff < 0) diff = -diff;

    if (diff < (int32_t)(estimated_bit_width / 6)) {
        estimated_bit_width = (estimated_bit_width * 7 + pulse_width) / 8;
        samples_per_bit = estimated_bit_width;
        candidate_hits = 0;
    } else {
        int32_t cand_diff = (int32_t)pulse_width - (int32_t)candidate_width;
        if (cand_diff < 0) cand_diff = -cand_diff;

        if (cand_diff < (int32_t)(candidate_width / 8)) {
            candidate_hits++;
            if (candidate_hits >= 3) {
                estimated_bit_width = (candidate_width + pulse_width) / 2;
                samples_per_bit = estimated_bit_width;
                candidate_hits = 0;
                uart_state = WAIT_START;
            }
        } else {
            candidate_width = pulse_width;
            candidate_hits = 1;
        }
    }
}

void RTTYRxProcessor::append_data(uint8_t raw_baudot_code) {
    if (tx_message.data_len < tx_message.max_len) {
        tx_message.data[tx_message.data_len] = raw_baudot_code;
        tx_message.data_len++;
    }
}

void RTTYRxProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::RTTYData) {
        const auto& rtty_msg = static_cast<const RTTYDataMessage&>(*message);

        if (rtty_msg.baud != baud_rate) {
            baud_rate = rtty_msg.baud;
            if (baud_rate > 0) {
                const float real_baud = (float)baud_rate / 100.0f;
                samples_per_bit = (uint32_t)((float)final_fs / real_baud);
                estimated_bit_width = samples_per_bit;
            } else {
                estimated_bit_width = 528;
                samples_per_bit = 528;
                inverted_polarity = false;
            }
            uart_state = WAIT_START;
        }
        shift_hz = rtty_msg.shift;
        if (!configured) {
            configure();
        }
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<RTTYRxProcessor>()};
    event_dispatcher.run();
    return 0;
}