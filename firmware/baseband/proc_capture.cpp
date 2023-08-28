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

using namespace dsp::decimate;

CaptureProcessor::CaptureProcessor() {
    // // What are these scale factors? Where did they come from?
    // decim_0_4.configure(taps_200k_decim_0.taps, 33'554'432);  // to be used with decim1 (/2), then total two decim stages decim (/8)
    // decim_0_8.configure(taps_200k_decim_0.taps, 33554432);  // to be used with decim1 (/2), then total two decim stages decim (/16)

    // // Why using 180k taps?
    // decim_0_8_180k.configure(taps_180k_wfm_decim_0.taps, 33554432);  // to be used alone - no additional decim1 (/2), then total single decim stage (/8)
    // decim_0_4_256.configure(taps_200k_decim_0.taps, 33554432);       // to be used alone - no additional decim1 (/2), then total single decim stage (/4)

    // // Why using 16k taps?
    // decim_1_2.configure(taps_200k_decim_1.taps, 131072);
    // decim_1_8.configure(taps_16k0_decim_1.taps, 131072);  // tentative decim1 /8 and taps, pending to be optimized.

    channel_spectrum.set_decimation_factor(1);  // What does this do?
    baseband_thread.start();
}

void CaptureProcessor::execute(const buffer_c8_t& buffer) {
    auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    auto out_buffer = decim_1.execute(decim_0_out, dst_buffer);

    // // 2.4576MHz, 2048 samples -- ??? Where did these come from?
    // const auto decim_0_out = decim_0_execute(buffer, dst_buffer);
    // const auto decim_1_out = decim_1_execute(decim_0_out, dst_buffer);

    // // ??? Why?
    // const auto& decimator_out = decim_1_out;
    // const auto& channel = decimator_out;

    // if (stream) {
    //     const size_t bytes_to_write = sizeof(*decimator_out.p) * decimator_out.count;
    //     const size_t written = stream->write(decimator_out.p, bytes_to_write);
    //     if (written != bytes_to_write) {
    //         // TODO: Report error somewhere?
    //     }
    // }

    // feed_channel_stats(channel);

    // spectrum_samples += channel.count;
    // if (spectrum_samples >= spectrum_interval_samples) {
    //     spectrum_samples -= spectrum_interval_samples;
    //     channel_spectrum.feed(channel, channel_filter_low_f, channel_filter_high_f, channel_filter_transition);
    // }
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
    // The actual sample rate is the requested rate * the oversample rate.
    // See oversample.hpp for more details on oversampling.
    baseband_fs = message.sample_rate * toUType(message.oversample_rate);
    oversample_rate = message.oversample_rate;

    baseband_thread.set_sampling_rate(baseband_fs);

    // TODO: Explain?
    constexpr int decim_0_scale = 33'554'432;
    constexpr int decim_1_scale = 131'072;

    // TODO: separate waterfall scalar to make it look nice?

    switch (message.oversample_rate) {
        case OversampleRate::x4:
            // M4 can't handle 2 decimation passes for sample rates needing x4.
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<NoopDecim>();
            break;

        case OversampleRate::x8:
            // M4 can't handle 2 decimation passes for sample rates at or above 600k.
            if (message.sample_rate < 600'00) {
                decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
                decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps, decim_1_scale);
            } else {
                // Using 180k taps to provide better filtering with a single pass.
                decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_180k_wfm_decim_0.taps, decim_0_scale);
                decim_1.set<NoopDecim>();
            }
            break;

        case OversampleRate::x16:
            decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps, decim_1_scale);
            break;

        case OversampleRate::x32:
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps, decim_1_scale);
            break;

        case OversampleRate::x64:
            decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps, decim_1_scale);
            break;

        default:
            chDbgPanic("Unhandled OversampleRate");
            break;
    }

    // // TODO: We should not need to be futzing around with the values to make the waterfall happy.

    // size_t decim_0_factor, decim_1_factor;
    // switch (oversample_rate) {  // we are using 3 decim_0 modes (/4,  /8-200k, /8-180k) , and   3 decim_1 modes : (/1 bypassed, /2,  /8)
    //     case OversampleRate::x4:
    //         decim_0_factor = decim_0_4.decimation_factor;  // /4 = /(4x1)

    // if (baseband_fs < 7'000'000) {  // we are in ovs x4 ,  < 1.5 Mhz , M4 is not overrun 100% when fs < 1M5 x 4 = 6000
    //     decim_1_factor = 2 * 1;     // decim1 is /1 because bypassed, (but just increased to adjust waterfall speed, it seems no effect to the write process or fft scale)
    // } else {
    //     decim_1_factor = 4 * 1;  // decim1 is /1 because bypassed ,(but just increased to adjust waterfall speed  when M4 has buffer drops )
    // }
    // break;

    // case OversampleRate::x8:
    //     if (baseband_fs < 4800'000) {
    //         decim_0_factor = decim_0_4.decimation_factor;  // /8 = /(4x2)
    //         decim_1_factor = decim_1_2.decimation_factor;  // /8 = /4x2,    factor 2 *  , it is just to speed up waterfall.
    //     } else {
    //         decim_0_factor = decim_0_8.decimation_factor;  // /8 = /(8x1),   600khz and onwards, single decim /8 = /8x1 (we applied additional *2 correction to speed up waterfall, no effect to scale spectrum)
    //         decim_1_factor = 1;                            // 600khz and onwards, single decim /8 = /8x1 (we applied additional *2 correction to speed up waterfall, no effect to scale spectrum)
    //     }
    //     break;

    // case OversampleRate::x16:
    //     decim_0_factor = decim_0_8.decimation_factor;      // /16 = /(8x2)
    //     decim_1_factor = 2 * decim_1_2.decimation_factor;  // /16 = /(8x2) (we applied additional *2 correction to increase waterfall spped and smooth & avoid abnormal motion )
    //     break;

    // case OversampleRate::x32:
    //     decim_0_factor = decim_0_4.decimation_factor;      // /32 = /(4x8) (we applied additional *2 correction to speed up waterfall, no effect to scale spectrum)
    //     decim_1_factor = 4 * decim_1_8.decimation_factor;  // /32 = /(4x8) (we applied additional *2 correction to speed up waterfall, no effect to scale spectrum)
    //     break;

    // case OversampleRate::x64:
    //     decim_0_factor = decim_0_8.decimation_factor;      // /64 = /(8x8) (we applied additional *8 correction to speed up waterfall, no effect to scale spectrum)
    //     decim_1_factor = 8 * decim_1_8.decimation_factor;  // /64 = /(8x8) (we applied additional *8 correction to speed up waterfall, no effect to scale spectrum)
    //     break;

    // default:
    //     decim_0_factor = 4;  // just default initial value to remove compile warning.
    //     decim_1_factor = 2;  // just default initial value to remove compile warning.
    //     break;

    // /*    // that was ok,  when we had only 2 oversampling x8, x16
    // auto decim_1_factor = oversample_rate == OversampleRate::x32
    //                           ? decim_1_8.decimation_factor
    //                           : decim_1_2.decimation_factor;
    // */

    // /*
    // // Test To limit to buffer dst 512 -> 256 samples to waterfall but it also changes wrongly the fft horiz. spectrum scale .
    // if (oversample_rate == OversampleRate::x4) {
    //     channel_spectrum.set_decimation_factor(2);
    // } else {
    //     channel_spectrum.set_decimation_factor(1);
    // }
    // */

    /*
    // TODO: Use the right taps per.
    size_t decim_0_output_fs = baseband_fs / decim_0_factor;
    size_t decim_1_input_fs = decim_0_output_fs;
    size_t decim_1_output_fs = decim_1_input_fs / decim_1_factor;

    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * decim_1_input_fs;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * decim_1_input_fs;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * decim_1_input_fs;

    spectrum_interval_samples = decim_1_output_fs / spectrum_rate_hz;
    spectrum_samples = 0;
    */
}

void CaptureProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        stream = std::make_unique<StreamInput>(message.config);
    } else {
        stream.reset();
    }
}

// buffer_c16_t CaptureProcessor::decim_0_execute(const buffer_c8_t& src, const buffer_c16_t& dst) {
//     switch (oversample_rate) {
//         case OversampleRate::x4:
//             return decim_0_4_256.execute(src, dst);  // decim_0, /4 with single  decim0 stage with limited output buffer c16 to 256 samples.

// case OversampleRate::x8:                     // we can get /8 by two means , decim0 (/4) + decim1 (/2) .  or just decim0 (/8)
//     if (baseband_fs < 4'800'000) {            // 600khz (600k x 8)
//         return decim_0_4.execute(src, dst);  // decim_0, /4 with double decim stage
//     } else {
//         return decim_0_8_180k.execute(src, dst);  // decim_0 /8 with single decim stage
//     }

// case OversampleRate::x16:
//     return decim_0_8.execute(src, dst);  // decim_0, /8 with double decim stage

// case OversampleRate::x32:
//     return decim_0_4.execute(src, dst);  // decim_0, /4 with double decim stage

// case OversampleRate::x64:
//     return decim_0_8.execute(src, dst);  // decim_0, /8 with double decim stage

// default:
//     chDbgPanic("Unhandled OversampleRate");
//     return {};}

// buffer_c16_t CaptureProcessor::decim_1_execute(const buffer_c16_t& src, const buffer_c16_t& dst) {
//     switch (oversample_rate) {
//         case OversampleRate::x4:
//             return src;  // just decim0_4 (/4)

// case OversampleRate::x8:           // we can get /8 by two means , decim0 (/4) + decim1 (/2) .  or just decim0 (/8)
//     if (baseband_fs < 4800'000) {  // 600khz (600k x 8)
//         return decim_1_2.execute(src, dst);
//     } else {
//         return src;  // just decim0_8 (/8)
//     }

// case OversampleRate::x16:
//     return decim_1_2.execute(src, dst);  // total decim /16 = /8x2, applied to 100khz and 150khz

// case OversampleRate::x32:
//     return decim_1_8.execute(src, dst);  // total decim /32 = /4x8, appled to  75k , 50k, 32k

// case OversampleRate::x64:
//     return decim_1_8.execute(src, dst);  // total decim /64 = /8x8, appled to 16k and 12k5

// default:
//     chDbgPanic("Unhandled OversampleRate");
//     return {};}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<CaptureProcessor>()};
    event_dispatcher.run();
    return 0;
}
