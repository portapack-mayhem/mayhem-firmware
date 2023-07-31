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
    decim_0_4.configure(taps_200k_decim_0.taps, 33554432);
    decim_0_8.configure(taps_200k_decim_0.taps, 33554432);
    decim_1.configure(taps_200k_decim_1.taps, 131072);

    channel_spectrum.set_decimation_factor(1);
    baseband_thread.start();
}

void CaptureProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */
    const auto decim_0_out = decim_0_execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
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

        case Message::ID::SamplerateConfig: {
            auto config = reinterpret_cast<const SamplerateConfigMessage*>(message);
            baseband_fs = config->sample_rate;
            update_for_rate_change();
            break;
        }

        case Message::ID::OversampleRateConfig: {
            auto config = reinterpret_cast<const OversampleRateConfigMessage*>(message);
            oversample_rate = config->oversample_rate;
            update_for_rate_change();
            break;
        }

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void CaptureProcessor::update_for_rate_change() {
    baseband_thread.set_sampling_rate(baseband_fs);

    auto decim_0_factor = oversample_rate == OversampleRate::Rate8x
                              ? decim_0_4.decimation_factor
                              : decim_0_8.decimation_factor;

    size_t decim_0_output_fs = baseband_fs / decim_0_factor;

    size_t decim_1_input_fs = decim_0_output_fs;
    size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

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
        case OversampleRate::Rate8x:
            return decim_0_4.execute(src, dst);

        case OversampleRate::Rate16x:
            return decim_0_8.execute(src, dst);

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
