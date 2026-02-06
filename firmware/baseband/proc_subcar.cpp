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
/*
   This and The other files related to this is based on a lot of great people's work. https://github.com/RocketGod-git/ProtoPirate Check the repo, and the credits inside.
*/
#include "proc_subcar.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

static inline int get_quadrant(int16_t i, int16_t q) {
    if (i >= 0) {
        return (q >= 0) ? 0 : 3;
    } else {
        return (q >= 0) ? 1 : 2;
    }
}

void SubCarProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // SR = 4Mhz ,  and we are decimating by /8 in total , decim1_out clock 4Mhz /8= 500khz samples/sec.
    // buffer has   2048 complex i8 I,Q signed samples
    // decim0 out:  2048/4 = 512 complex i16 I,Q signed samples
    // decim1 out:  512/2 =  256 complex i16 I,Q signed samples
    // Regarding Filters, we are re-using existing FIR filters, @4Mhz, FIR decim1 ilter, BW =+-220Khz (at -3dB's). BW = 440kHZ.

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);       // Input:2048 complex/4 (decim factor) = 512_output complex (1024 I/Q samples)
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);  // Input:512  complex/2 (decim factor) = 256_output complex ( 512 I/Q samples)
    feed_channel_stats(decim_1_out);

    // for fm
    const int32_t DC_ALPHA = 5;  // Auto-centering speed
    int32_t buffer_rotation_sum = 0;

    for (size_t i = 0; i < decim_1_out.count; i++) {
        // am
        threshold = (low_estimate + high_estimate) / 2;
        int32_t const hysteresis = threshold / 8;  // +-12%
        int16_t re = decim_1_out.p[i].real();
        int16_t im = decim_1_out.p[i].imag();
        uint32_t mag = ((uint32_t)re * (uint32_t)re) + ((uint32_t)im * (uint32_t)im);

        mag = (mag >> 10);
        int32_t const ook_low_delta = mag - low_estimate;
        bool meashl = currentHiLow;
        if (sig_state == STATE_IDLE) {
            if (mag > (threshold + hysteresis)) {  // just become high
                meashl = true;
                sig_state = STATE_PULSE;
                numg = 0;
            } else {
                meashl = false;  // still low
                low_estimate += ook_low_delta / OOK_EST_LOW_RATIO;
                low_estimate += ((ook_low_delta > 0) ? 1 : -1);  // Hack to compensate for lack of fixed-point scaling
                // Calculate default OOK high level estimate
                high_estimate = 1.35 * low_estimate;  // Default is a ratio of low level
                high_estimate = std::max(high_estimate, min_high_level);
                high_estimate = std::min(high_estimate, (uint32_t)OOK_MAX_HIGH_LEVEL);
            }

        } else if (sig_state == STATE_PULSE) {
            ++numg;
            if (numg > 100) numg = 100;
            if (mag < (threshold - hysteresis)) {
                // check if really a bad value
                if (numg < 3) {
                    // susp
                    sig_state = STATE_GAP;
                } else {
                    numg = 0;
                    sig_state = STATE_GAP_START;
                }
                meashl = false;  // low
            } else {
                high_estimate += mag / OOK_EST_HIGH_RATIO - high_estimate / OOK_EST_HIGH_RATIO;
                high_estimate = std::max(high_estimate, min_high_level);
                high_estimate = std::min(high_estimate, (uint32_t)OOK_MAX_HIGH_LEVEL);
                meashl = true;  // still high
            }
        } else if (sig_state == STATE_GAP_START) {
            ++numg;
            if (mag > (threshold + hysteresis)) {  // New pulse?
                sig_state = STATE_PULSE;
                meashl = true;
            } else if (numg >= 3) {
                sig_state = STATE_GAP;
                meashl = false;  // gap
            }
        } else if (sig_state == STATE_GAP) {
            ++numg;
            if (mag > (threshold + hysteresis)) {  // New pulse?
                numg = 0;
                sig_state = STATE_PULSE;
                meashl = true;
            } else {
                meashl = false;
            }
        }

        if (meashl == currentHiLow && currentDuration < 30'000'000)  // allow pass 'end' signal
        {
            currentDuration += nsPerDecSamp;
        } else {  // called on change, so send the last duration and dir.
            if (currentDuration >= 30'000'000) sig_state = STATE_IDLE;
            if (protoList) protoList->feed(currentHiLow, currentDuration / 1000);
            currentDuration = nsPerDecSamp;
            currentHiLow = meashl;
        }

        // fm part: -- NOT WORKING!!!! TODO FIX. AI code ;)
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
        // Update History
        fm_state.prev_quad = current_quad;
        // Accumulate Rotation
        buffer_rotation_sum += diff;
    }

    // fm finish:
    //  3. AUTO-CENTERING (DC BLOCKER)
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
            if (protoListFm) protoListFm->feed(fm_state.current_logic_level, duration_us);
        }
        fm_state.current_logic_level = new_level;
        fm_state.buffer_count = 1;
    }
}

void SubCarProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::SubGhzFPRxConfigure)
        configure(*reinterpret_cast<const SubGhzFPRxConfigureMessage*>(message));
}

void SubCarProcessor::configure(const SubGhzFPRxConfigureMessage& message) {
    // constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor; //unused
    // constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor; //unused

    baseband_fs = message.sampling_rate;
    baseband_thread.set_sampling_rate(baseband_fs);
    nsPerDecSamp = 1'000'000'000 / baseband_fs * 8;  // Scaled it due to less array buffer sampes due to /8 decimation.  250 nseg (4Mhz) * 8

    decim_0.configure(taps_200k_wfm_decim_0.taps);
    decim_1.configure(taps_200k_wfm_decim_1.taps);

    configured = true;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SubCarProcessor>()};
    event_dispatcher.run();
    return 0;
}
