/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "proc_protoview.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"

static inline int get_quadrant(int16_t i, int16_t q) {
    if (i >= 0) {
        return (q >= 0) ? 0 : 3;
    } else {
        return (q >= 0) ? 1 : 2;
    }
}

void ProtoViewProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // 1. Decimation Chain
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    feed_channel_stats(decim_1_out);

    // --- CONFIGURATION ---
    const int32_t DC_ALPHA = 5;  // Auto-centering speed
    int32_t buffer_rotation_sum = 0;

    // 2. QUADRANT COUNTING LOOP
    for (size_t k = 0; k < decim_1_out.count; k++) {
        int16_t re = decim_1_out.p[k].real();
        int16_t im = decim_1_out.p[k].imag();

        // Get current quadrant (0, 1, 2, or 3)
        int current_quad = get_quadrant(re, im);

        // Calculate Step (Current - Previous)
        int diff = current_quad - fm_state.prev_quad;

        // Handle Wrap-Around (crossing from Q3 to Q0 or Q0 to Q3)
        //  3 -> 0 should be +1 (CCW)
        //  0 -> 3 should be -1 (CW)
        if (diff == -3)
            diff = 1;
        else if (diff == 3)
            diff = -1;

        // Sanity Check: If diff is +/- 2, we skipped a quadrant (aliasing or noise).
        // We usually ignore it or guess the direction based on history.
        // For simplicity, treat +/- 2 as 0 (invalid step).
        if (diff == 2 || diff == -2) diff = 0;

        // Update History
        fm_state.prev_quad = current_quad;

        // Accumulate Rotation
        buffer_rotation_sum += diff;
    }

    // 3. AUTO-CENTERING (DC BLOCKER)
    // Even with quadrant counting, "drift" (hand effect) makes the wheel spin
    // faster or slower. We need to subtract the average speed.

    // Update our "Average Speed" estimate
    // Note: buffer_rotation_sum is roughly proportional to frequency.
    fm_state.dc_offset = (fm_state.dc_offset * ((1 << DC_ALPHA) - 1) + buffer_rotation_sum) >> DC_ALPHA;

    // Remove the drift
    int32_t centered_rotation = buffer_rotation_sum - fm_state.dc_offset;

    // 4. LOW PASS FILTER
    const int32_t LPF_ALPHA = 4;
    fm_state.smoothed_error = (fm_state.smoothed_error * (LPF_ALPHA - 1) + centered_rotation) / LPF_ALPHA;

    // 5. DECISION LOGIC
    // Threshold is small now because we are counting quadrant steps.
    // Max steps per buffer (256 samples) is 256.
    // Typical FSK deviation might give you +/- 10 to 50 steps per buffer.
    const int32_t THRESHOLD = 3;

    bool new_level = fm_state.current_logic_level;

    if (fm_state.smoothed_error > THRESHOLD) {
        new_level = true;
    } else if (fm_state.smoothed_error < -THRESHOLD) {
        new_level = false;
    }

    // 6. TIMING OUTPUT
    if (new_level == fm_state.current_logic_level) {
        fm_state.buffer_count++;
    } else {
        // Output pulse duration
        int32_t duration_us = fm_state.buffer_count * 512;

        if (duration_us > 250) {
            message.times[message.timeptr++] = fm_state.current_logic_level ? duration_us : -duration_us;
            if (message.timeptr > message.maxptr) {
                shared_memory.application_queue.push(message);
                message.timeptr = 0;
            }
        }
        fm_state.current_logic_level = new_level;
        fm_state.buffer_count = 1;
    }
}

void ProtoViewProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::SubGhzFPRxConfigure:
            configure(*reinterpret_cast<const SubGhzFPRxConfigureMessage*>(message));
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(message));
            break;

        default:
            break;
    }
}

void ProtoViewProcessor::configure(const SubGhzFPRxConfigureMessage& message) {
    baseband_fs = message.sampling_rate;
    baseband_thread.set_sampling_rate(baseband_fs);
    nsPerDecSamp = 1'000'000'000 / baseband_fs * 8;  // Scaled it due to less array buffer sampes due to /8 decimation.  250 nseg (4Mhz) * 8

    // constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor; //unused
    // constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor; //unused

    decim_0.configure(taps_200k_wfm_decim_0.taps);
    decim_1.configure(taps_200k_wfm_decim_1.taps);

    configured = true;
}

void ProtoViewProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<ProtoViewProcessor>()};
    event_dispatcher.run();
    return 0;
}
