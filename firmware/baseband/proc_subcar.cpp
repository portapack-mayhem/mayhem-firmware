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

    for (size_t i = 0; i < decim_1_out.count; i++) {
        // am
        threshold = (low_estimate + high_estimate) / 2;
        int32_t const hysteresis = threshold / 8;  // +-12%
        int16_t re = decim_1_out.p[i].real();
        int16_t im = decim_1_out.p[i].imag();
        uint32_t mag = ((uint32_t)re * (uint32_t)re) + ((uint32_t)im * (uint32_t)im);
        mag = (mag >> 10);
        if (modulation == 0) {
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
        }
        if (modulation == 1) {
            int32_t discrim = ((int32_t)im * fm_state.last_re) - ((int32_t)re * fm_state.last_im);
            fm_state.last_re = re;
            fm_state.last_im = im;
            fm_state.smoothed_discrim += (discrim - fm_state.smoothed_discrim) >> 4;

            // --- FM Part (Simple 2-FSK) ---
            if (mag > (threshold / 2)) {
                const int32_t fm_hysteresis = 2000;
                bool new_level = fm_state.current_logic_level;
                if (fm_state.smoothed_discrim > fm_hysteresis) {
                    new_level = true;
                } else if (fm_state.smoothed_discrim < -fm_hysteresis) {
                    new_level = false;
                }
                if (new_level == fm_state.current_logic_level) {
                    fm_state.buffer_count++;
                } else {
                    int32_t duration_us = (fm_state.buffer_count * nsPerDecSamp) / 1000;
                    if (duration_us > 15) {
                        if (protoList) protoList->feed(fm_state.current_logic_level, duration_us);
                    }
                    fm_state.current_logic_level = new_level;
                    fm_state.buffer_count = 1;
                }
            } else {
                fm_state.buffer_count = 0;
            }
        }
    }
}

void SubCarProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::SubGhzFPRxConfigure)
        configure(*reinterpret_cast<const SubGhzFPRxConfigureMessage*>(message));
}

void SubCarProcessor::configure(const SubGhzFPRxConfigureMessage& message) {
    // constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor; //unused
    // constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor; //unused

    if (modulation != message.modulation) {
        // reload protos to reset them all
        if (protoList) {
            delete protoList;
        }
        protoList = new SubCarProtos();
    }
    modulation = message.modulation;
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
