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

#include "proc_wefaxrx.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "audio_dma.hpp"
#include "math.h"
#include "event_m4.hpp"
#include "fxpt_atan2.hpp"

#include <cstdint>
#include <cstddef>

#define STARTSIGNAL_TH 0.33
#define STARTSIGNAL_NEEDCNT 110
#define STARTSIGNAL_MAXBAD 20

#define WEFAX_PX_SIZE 840.0

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
    pxRem = (double)channel_filter_input_fs / ((lpm / 60.0) * WEFAX_PX_SIZE);
    samples_per_pixel = pxRem;
    pxRem -= samples_per_pixel;
    pxRoll = 0;
    status_message.state = 0;
    shared_memory.application_queue.push(status_message);
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
    auto audio = demod_ssb_fm.execute(channel_out, audio_buffer);
    audio_compressor.execute_in_place(audio);
    audio_output.write(audio);

    for (size_t c = 0; c < audio.count; c++) {
        if (status_message.state == 0 && false) {  // disabled this due to so sensitive to noise
            // first look for the sync!
            if (audio.p[c] <= STARTSIGNAL_TH && audio.p[c] >= 0.0001) {
                sync_cnt++;
                if (sync_cnt >= STARTSIGNAL_NEEDCNT) {
                    status_message.state = 1;
                    shared_memory.application_queue.push(status_message);
                    sync_cnt = 0;
                    syncnot_cnt = 0;
                }
            } else {
                syncnot_cnt++;
                if (syncnot_cnt >= STARTSIGNAL_MAXBAD) {
                    sync_cnt = 0;
                    syncnot_cnt = 0;
                }
            }
        } else {
            cnt++;
            if (cnt >= (samples_per_pixel + (uint32_t)pxRoll)) {  // got a pixel
                cnt = 0;
                if (pxRoll >= 1) pxRoll -= 1.0;
                pxRoll += pxRem;

                if (image_message.cnt < 400) {
                    if (audio.p[c] >= 0.68) {
                        image_message.image[image_message.cnt++] = 255;
                    } else if (audio.p[c] >= 0.45) {
                        image_message.image[image_message.cnt++] = (uint8_t)(((audio.p[c] - 0.45f) * 1108));
                    } else {
                        image_message.image[image_message.cnt++] = 0;
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

    channel_filter_input_fs = decim_2_output_fs;
    // const size_t channel_filter_output_fs = channel_filter_input_fs / channel_filter_decimation_factor;

    decim_0.configure(taps_6k0_decim_0.taps);
    decim_1.configure(taps_6k0_decim_1.taps);
    decim_2.configure(taps_6k0_decim_2.taps, decim_2_decimation_factor);
    channel_filter.configure(taps_2k6_usb_wefax_channel.taps, channel_filter_decimation_factor);
    channel_filter_low_f = taps_2k6_usb_wefax_channel.low_frequency_normalized * channel_filter_input_fs;
    channel_filter_high_f = taps_2k6_usb_wefax_channel.high_frequency_normalized * channel_filter_input_fs;
    channel_filter_transition = taps_2k6_usb_wefax_channel.transition_normalized * channel_filter_input_fs;
    channel_spectrum.set_decimation_factor(1.0f);
    audio_output.configure(apt_audio_12k_lpf_1500hz_config);  // hpf in all AM demod modes (AM-6K/9K, USB/LSB,DSB), except Wefax (lpf there).

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
