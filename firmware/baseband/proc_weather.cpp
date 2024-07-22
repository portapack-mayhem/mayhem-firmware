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

#include "proc_weather.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"

void WeatherProcessor::execute(const buffer_c8_t& buffer) {
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
        int16_t re = decim_1_out.p[i].real();
        int16_t im = decim_1_out.p[i].imag();
        uint32_t mag = ((uint32_t)re * (uint32_t)re) + ((uint32_t)im * (uint32_t)im);

        mag = (mag >> 12);  // Decim samples are calculated with saturated gain . (we could also reduce that sat. param at configure time)

        bool meashl = (mag > threshold);
        tm += mag;
        if (meashl == currentHiLow && currentDuration < 30'000'000)  // allow pass 'end' signal
        {
            currentDuration += nsPerDecSamp;
        } else {  // called on change, so send the last duration and dir.
            if (protoList) protoList->feed(currentHiLow, currentDuration / 1000);
            currentDuration = nsPerDecSamp;
            currentHiLow = meashl;
        }
    }

    cnt += decim_1_out.count;  // TODO , check if it is necessary that xdecim factor.
    if (cnt > 90'000) {
        threshold = (tm / cnt) / 2;
        cnt = 0;
        tm = 0;
        if (threshold < 50) threshold = 50;
        if (threshold > 1700) threshold = 1700;
    }
}

void WeatherProcessor::on_message(const Message* const message) {
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

void WeatherProcessor::configure(const SubGhzFPRxConfigureMessage& message) {
    baseband_fs = message.sampling_rate;
    baseband_thread.set_sampling_rate(baseband_fs);
    nsPerDecSamp = 1'000'000'000 / baseband_fs * 8;  // Scaled it due to less array buffer sampes due to /8 decimation.  250 nseg (4Mhz) * 8

    // constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor; //unused
    // constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor; //unused

    decim_0.configure(taps_200k_wfm_decim_0.taps);
    decim_1.configure(taps_200k_wfm_decim_1.taps);

    configured = true;
}

void WeatherProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<WeatherProcessor>()};
    event_dispatcher.run();
    return 0;
}
