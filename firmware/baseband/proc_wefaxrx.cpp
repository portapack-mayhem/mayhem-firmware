/*
 * Copyright (C) 2024 HTotoo
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

#include "proc_wefaxrx.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "audio_dma.hpp"
#include "math.h"
#include "event_m4.hpp"
#include "fxpt_atan2.hpp"

#include <cstdint>
#include <cstddef>

#define M_PI 3.14159265358979323846

// updates the per pixel timers
void WeFaxRx::update_params() {
    switch (ioc_mode) {
        case 1:
            freq_start_tone = 675;
            break;
        default:
        case 0:
            freq_start_tone = 300;
            break;
    }
    // 840 px / line with line start
    time_per_pixel = 60000000 / lpm * 840;                              // micros (595,2380952 at 120 lpm)
    pxRem = (double)baseband_fs / 8.0 / 8.0 / 4.0 / ((int)lpm * 14.0);  // 840/60  = 228.57 sample / px
    samples_per_pixel = pxRem;
    pxRem -= samples_per_pixel;
    pxRoll = 0;
}

void WeFaxRx::execute(const buffer_c8_t& buffer) {
    // This is called at 3072000 / 2048 = 1500Hz
    if (!configured) return;
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    channel_spectrum.feed(decim_1_out, channel_filter_low_f, channel_filter_high_f, channel_filter_transition);

    const auto decim_2_out = decim_2.execute(decim_1_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_2_out, dst_buffer);

    feed_channel_stats(channel_out);
    auto audio = demodulate(channel_out);  // now 3 AM demodulation types : demod_am, demod_ssb, demod_ssb_fm (for Wefax)
    audio_compressor.execute_in_place(audio);
    audio_output.write(audio);
    // todo process

    /*
    for (size_t c = 0; c < decim_2_out.count; c++) {
        cnt++;
        double freqq = calculateFrequencyDeviation(decim_2_out.p[c], iqlast);
        if (status_message.freqmin > freqq) status_message.freqmin = freqq;
        if (status_message.freqmax < freqq) status_message.freqmax = freqq;
        status_message.freqavg += (freqq - status_message.freqavg) / cnt;
        iqlast = decim_2_out.p[c];
        if (cnt >= (samples_per_pixel + (uint32_t)pxRoll)) {  // got a pixel
            cnt = 0;
            if (pxRoll >= 1) pxRoll -= 1;
            pxRoll += pxRem;
            status_message.freq = freqq;
            image_message.cnt++;  // saves the pixel
            if (image_message.cnt < 400) {
                image_message.image[image_message.cnt] = status_message.freqavg < 2500 ? 0 : 255;  // todo remove limit, send in multiple
                /*if (status_message.freqavg >= 3000)
                    image_message.image[image_message.cnt] = 255;
                else if (status_message.freqavg <= 2200)
                    image_message.image[image_message.cnt] = 0;
                else {
                    image_message.image[image_message.cnt] = 256 - ((3000 - status_message.freqavg) / 3.1);
                }* /
            }
            if (image_message.cnt >= 399) {
                shared_memory.application_queue.push(image_message);
                image_message.cnt = 0;
                shared_memory.application_queue.push(status_message);
                status_message.freqmin = INT32_MAX;
                status_message.freqmax = INT32_MIN;
            }
            status_message.freqavg = 0;
        }
    } */
}

buffer_f32_t WeFaxRx::demodulate(const buffer_c16_t& channel) {
    return demod_ssb_fm.execute(channel, audio_buffer);  // Calling a derivative of demod_ssb (USB) , but with different FIR taps + FM audio tones demod.
}

void WeFaxRx::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(message);
            break;

        case Message::ID::WeFaxRxConfigure:
            configure(*reinterpret_cast<const WeFaxRxConfigureMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void WeFaxRx::configure(const WeFaxRxConfigureMessage& message) {
    constexpr size_t decim_0_input_fs = baseband_fs;
    constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

    constexpr size_t decim_1_input_fs = decim_0_output_fs;
    constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

    constexpr size_t decim_2_input_fs = decim_1_output_fs;
    constexpr size_t decim_2_output_fs = decim_2_input_fs / decim_2_decimation_factor;

    constexpr size_t channel_filter_input_fs = decim_2_output_fs;
    // const size_t channel_filter_output_fs = channel_filter_input_fs / channel_filter_decimation_factor;

    decim_0.configure(taps_6k0_decim_0.taps);
    decim_1.configure(taps_6k0_decim_1.taps);
    decim_2.configure(taps_6k0_decim_2.taps, decim_2_decimation_factor);
    channel_filter.configure(taps_2k6_usb_wefax_channel.taps, channel_filter_decimation_factor);
    channel_filter_low_f = taps_2k6_usb_wefax_channel.low_frequency_normalized * channel_filter_input_fs;
    channel_filter_high_f = taps_2k6_usb_wefax_channel.high_frequency_normalized * channel_filter_input_fs;
    channel_filter_transition = taps_2k6_usb_wefax_channel.transition_normalized * channel_filter_input_fs;
    channel_spectrum.set_decimation_factor(1.0f);
    // modulation_ssb = (message.modulation == AMConfigureMessage::Modulation::SSB);  // originally we had just 2 AM types of demod. (DSB , SSB)
    modulation_ssb = (int)2;                              // now sending by message , 3 types of AM demod :   enum class Modulation : int32_t {DSB = 0, SSB = 1, SSB_FM = 2}
    audio_output.configure(audio_12k_lpf_1500hz_config);  // hpf in all AM demod modes (AM-6K/9K, USB/LSB,DSB), except Wefax (lpf there).

    lpm = message.lpm;
    ioc_mode = message.ioc;

    update_params();
    configured = true;
}

void WeFaxRx::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<WeFaxRx>()};
    event_dispatcher.run();
    return 0;
}
