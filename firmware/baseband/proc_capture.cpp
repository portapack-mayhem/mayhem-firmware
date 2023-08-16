/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "proc_capture.hpp"

#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"
#include "utility.hpp"

CaptureProcessor::CaptureProcessor() {
    decim_0_4.configure(taps_200k_decim_0.taps, 33554432);           // to be used with decim1 (/2), then total two stages decim (x8)
    decim_0_8.configure(taps_200k_decim_0.taps, 33554432);           // to be used with decim1 (/2), then total two stages decim (x16)
    decim_0_8_180k.configure(taps_180k_wfm_decim_0.taps, 33554432);  // to be used alone - no additional decim1 (/2), then total single stage decim (x8)

    decim_1_2.configure(taps_200k_decim_1.taps, 131072);
    decim_1_8.configure(taps_16k0_decim_1.taps, 131072);  // tentative decim1 and   taps x8 , pending to be optimized.

    channel_spectrum.set_decimation_factor(1);
    baseband_thread.start();
}

void CaptureProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */
    const auto decim_0_out = decim_0_execute(buffer, dst_buffer);       // selectable 3 possible decim_0, (x4.  x8 200k soft filter , x8 180k sharp )
    const auto decim_1_out = decim_1_execute(decim_0_out, dst_buffer);  // selectable 3 possible decim_1, (x8.  /2 200k or bypassed /1  )

    /* this code was valid when we had only 2 decim1 cases.
    const auto decim_1_out = baseband_fs < 4800'000
                                  ? decim_1_2.execute(decim_0_out, dst_buffer)  // < 600khz double decim. stage , means 500khz and lower bit rates.
                                 // ? decim_1_8.execute(decim_0_out, dst_buffer)  // < 600khz double decim. stage , means 500khz and lower bit rates.
                                  : decim_0_out;                              // >= 600khz single decim. stage , means 600khz and upper bit rates.

     } */

    const auto& decimator_out = decim_1_out;
    const auto& channel = decimator_out;

    if (stream) {
        const size_t bytes_to_write = sizeof(*decimator_out.p) * decimator_out.count;
        const size_t written = stream->write(decimator_out.p, bytes_to_write);
        if (written != bytes_to_write) {
            // TODO eventually report error somewhere
        }
    }

    feed_channel_stats(channel);

    spectrum_samples += channel.count;
    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        channel_spectrum.feed(channel, channel_filter_low_f, channel_filter_high_f, channel_filter_transition);
    }
}

void CaptureProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(message);
            break;

        case Message::ID::SampleRateConfig:
            sample_rate_config(*reinterpret_cast<const SampleRateConfigMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void CaptureProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    baseband_fs = message.sample_rate * toUType(message.oversample_rate);
    oversample_rate = message.oversample_rate;

    baseband_thread.set_sampling_rate(baseband_fs);

    // Current fw , we are using only  2 decim_0 modes,  x4 , x8
    auto decim_0_factor = oversample_rate == OversampleRate::x8
                              ? decim_0_4.decimation_factor
                              : decim_0_8.decimation_factor;

    size_t decim_0_output_fs = baseband_fs / decim_0_factor;
    size_t decim_1_input_fs = decim_0_output_fs;

    size_t decim_1_factor;
    switch (oversample_rate) {  // we are using 3 decim_1 modes,  x1 , x2 ,  x8
        case OversampleRate::x8:
            if (baseband_fs < 4800'000) {
                decim_1_factor = decim_1_2.decimation_factor;  // x8 = 4x2
            } else {
                decim_1_factor = 2*1;  // 600khz and onwards, single decim x8 = 8x1 (we applied additional *2 correction to speed up waterfall, no effect to scale spectrum)
            }
            break;

        case OversampleRate::x16:
            decim_1_factor = 2 * decim_1_2.decimation_factor;  // x16 = 8x2 (we applied additional *2 correction to increase waterfall spped >=600k and smooth & avoid abnormal motion >1M5 )
            break;

        case OversampleRate::x32:
            decim_1_factor = 2 * decim_1_8.decimation_factor;  // x32 = 4x8 (we applied additional *2 correction to speed up waterfall, no effect to scale spectrum)
            break;

        default:
            decim_1_factor = 2;  // just default initial value to remove compile warning.
            break;
    }

    /*
    auto decim_1_factor = oversample_rate == OversampleRate::x32    // that was ok,  when we had only 2 oversampling x8 , x16
                              ? decim_1_8.decimation_factor
                              : decim_1_2.decimation_factor;

    */
    size_t decim_1_output_fs = decim_1_input_fs / decim_1_factor;

    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * decim_1_input_fs;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * decim_1_input_fs;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * decim_1_input_fs;

    spectrum_interval_samples = decim_1_output_fs / spectrum_rate_hz;
    spectrum_samples = 0;
}

void CaptureProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        stream = std::make_unique<StreamInput>(message.config);
    } else {
        stream.reset();
    }
}

buffer_c16_t CaptureProcessor::decim_0_execute(const buffer_c8_t& src, const buffer_c16_t& dst) {
    switch (oversample_rate) {
        case OversampleRate::x8:                     // we can get x8 by two means , decim0 (x4) + decim1 (x2) .  or just decim0 (x8)
            if (baseband_fs < 4800'000) {            // 600khz (600k x 8)
                return decim_0_4.execute(src, dst);  // decim_0  x4 with double decim stage
            } else {
                return decim_0_8_180k.execute(src, dst);  // decim_0  x8 with single decim stage
            }

        case OversampleRate::x16:
            return decim_0_8.execute(src, dst);  // decim_0  x8 with double decim stage

        case OversampleRate::x32:
            return decim_0_4.execute(src, dst);  // decim_0  x4 with double decim stage

        default:
            chDbgPanic("Unhandled OversampleRate");
            return {};
    }
}

buffer_c16_t CaptureProcessor::decim_1_execute(const buffer_c16_t& src, const buffer_c16_t& dst) {
    switch (oversample_rate) {
        case OversampleRate::x8:           // we can get x8 by two means , decim0 (x4) + decim1 (x2) .  or just decim0 (x8)
            if (baseband_fs < 4800'000) {  // 600khz (600k x 8)
                return decim_1_2.execute(src, dst);
            } else {
                return src;
            }

        case OversampleRate::x16:
            return decim_1_2.execute(src, dst);  // 8x2  ,150khz

        case OversampleRate::x32:
            return decim_1_8.execute(src, dst);  // 4x8 , 75khz to 100khz.

        default:
            chDbgPanic("Unhandled OversampleRate");
            return {};
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<CaptureProcessor>()};
    event_dispatcher.run();
    return 0;
}
