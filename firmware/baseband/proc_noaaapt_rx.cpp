/*
 * Copyright (C) 2025 Brumi, HTotoo
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

#include "proc_noaaapt_rx.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "audio_dma.hpp"
#include "math.h"
#include "event_m4.hpp"
#include "fxpt_atan2.hpp"

#include <cstdint>
#include <cstddef>

#define NOAAAPT_PX_SIZE 2080.0

// updates the per pixel timers
void NoaaAptRx::update_params() {
    // TODO HTOTOO
    //  2080 px / line with chan a,b + sync + telemetry
    pxRem = (double)12000.0 / NOAAAPT_PX_SIZE / 2.0;
    samples_per_pixel = pxRem;
    pxRem -= samples_per_pixel;
    pxRoll = 0;
    status_message.state = 0;
    shared_memory.application_queue.push(status_message);
}

void NoaaAptRx::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        return;
    }
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto channel = decim_1.execute(decim_0_out, dst_buffer);
    // TODO: Feed channel_stats post-decimation data?
    feed_channel_stats(channel);

    /* spectrum_samples += channel.count;
    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        channel_spectrum.feed(channel, channel_filter_low_f, channel_filter_high_f, channel_filter_transition);
    }
    */
    /* 96kHz complex<int16_t>[64]  for wfmam NOAA
     * -> FM demodulation
     * -> 96kHz int16_t[64] */
    auto audio_oversampled = demod.execute(channel, work_audio_buffer);  // fs 384khz wfm , 96khz wfmam for NOAA
    /* 96kHz int16_t[64]     for wfam
     * -> 4th order CIC decimation by 2, gain of 1
     * -> 48kHz int16_t[32] */
    auto audio_4fs = audio_dec_1.execute(audio_oversampled, work_audio_buffer);
    /* 48kHz int16_t[32]     for wfman
     * -> 4th order CIC decimation by 2, gain of 1
     * -> 24kHz int16_t[16] */
    auto audio_2fs = audio_dec_2.execute(audio_4fs, work_audio_buffer);
    /* 24kHz int16_t[16]          for wfmam
     * -> FIR filter, <4.5kHz (0.1875fs) pass, >5.2kHz (0.2166fs) stop, gain of 1
     * -> 12kHz int16_t[8] */
    auto audio = audio_filter.execute(audio_2fs, work_audio_buffer);
    /* -> 12kHz int16_t[8]   for wfmam ,  */
    std::array<float, 32> audio_f;
    audio_output.apt_write(audio, audio_f);  // we are in added wfmam (noaa), decim_1.decimation_factor == 8

    for (size_t c = 0; c < audio.count; c++) {
        if (status_message.state == 0 && false) {  // disabled this due to NIY
            // first look for the sync!

        } else {
            cnt++;
            if (cnt >= (samples_per_pixel + (uint32_t)pxRoll)) {  // got a pixel
                cnt = 0;
                if (pxRoll >= 1) pxRoll -= 1.0;
                pxRoll += pxRem;

                if (image_message.cnt < 400) {
                    if (audio_f[c] >= 1) {
                        image_message.image[image_message.cnt++] = 255;
                    } else if (audio_f[c] <= 0) {
                        image_message.image[image_message.cnt++] = 0;
                    } else {
                        image_message.image[image_message.cnt++] = (audio_f[c]) * 255;
                    }
                }
                if (image_message.cnt >= 399) {
                    shared_memory.application_queue.push(image_message);
                    image_message.cnt = 0;
                    if (status_message.state != 2) {
                        status_message.state = 2;
                        shared_memory.application_queue.push(status_message);
                    }
                }
            }
        }
    }
}

void NoaaAptRx::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            // channel_spectrum.on_message(message);
            break;

        case Message::ID::NoaaAptRxConfigure:
            configure(*reinterpret_cast<const NoaaAptRxConfigureMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void NoaaAptRx::configure(const NoaaAptRxConfigureMessage& message) {
    (void)message;
    constexpr size_t decim_0_input_fs = baseband_fs;
    constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;
    constexpr size_t decim_1_input_fs = decim_0_output_fs;

    decim_0.configure(taps_16k0_decim_0.taps);

    // decim_1.configure(message.decim_1_filter.taps);  // Original .
    // TODO dynamic decim1 ,  with decimation 2 / 8 and  16 x taps , / 32 taps .
    // Temptatively , I splitted, in two WidebandFMAudio::configure_wfm / WidebandFMAudio::configure_wfmam  and dynamically /2, /8 . (here /8)
    // decim_1.set<dsp::decimate::FIRC16xR16x16Decim2>().configure(message.decim_1_filter.taps);  // for wfm
    // decim_1.set<dsp::decimate::FIRC16xR16x32Decim8>().configure(message.decim_1_filter.taps);    // for wfmam
    decim_1.configure(taps_84k_wfmam_decim_1.taps);                           // for wfmam
    size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;  // wfmam,  decim_1.decimation_factor() = /8 ,if applied after the line, decim_1.set<dsp::decimate::FIRC16xR16x16Decim2>().configure(message.decim_1_filter.taps);
    size_t demod_input_fs = decim_1_output_fs;

    // spectrum_interval_samples = decim_1_output_fs / spectrum_rate_hz;
    // spectrum_samples = 0;

    channel_filter_low_f = taps_84k_wfmam_decim_1.low_frequency_normalized * decim_1_input_fs;
    channel_filter_high_f = taps_84k_wfmam_decim_1.high_frequency_normalized * decim_1_input_fs;
    channel_filter_transition = taps_84k_wfmam_decim_1.transition_normalized * decim_1_input_fs;
    demod.configure(demod_input_fs, 17000);
    audio_filter.configure(taps_64_lp_1875_2166.taps);
    audio_output.configure(apt_audio_12k_notch_2k4_config, apt_audio_12k_lpf_2000hz_config);
    // channel_spectrum.set_decimation_factor(1);
    update_params();
    configured = true;
}

void NoaaAptRx::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<NoaaAptRx>()};
    event_dispatcher.run();
    return 0;
}
