/*
 * Copyright (C) 1996 Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 * Copyright (C) 2012-2014 Elias Oenal (multimon-ng@eliasoenal.com)
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

#include "proc_fsk_rx.hpp"

#include "event_m4.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

using namespace std;
using namespace dsp::decimate;

namespace {
/* Count of bits that differ between the two values. */
uint8_t diff_bit_count(uint32_t left, uint32_t right) {
    uint32_t diff = left ^ right;
    uint8_t count = 0;
    for (size_t i = 0; i < sizeof(diff) * 8; ++i) {
        if (((diff >> i) & 0x1) == 1)
            ++count;
    }

    return count;
}
}  // namespace

/* AudioNormalizer ***************************************/

void AudioNormalizer::execute_in_place(const buffer_f32_t& audio) {
    // Decay min/max every second (@24kHz).
    if (counter_ >= 24'000) {
        // 90% decay factor seems to work well.
        // This keeps large transients from wrecking the filter.
        max_ *= 0.9f;
        min_ *= 0.9f;
        counter_ = 0;
        calculate_thresholds();
    }

    counter_ += audio.count;

    for (size_t i = 0; i < audio.count; ++i) {
        auto& val = audio.p[i];

        if (val > max_) {
            max_ = val;
            calculate_thresholds();
        }
        if (val < min_) {
            min_ = val;
            calculate_thresholds();
        }

        if (val >= t_hi_)
            val = 1.0f;
        else if (val <= t_lo_)
            val = -1.0f;
        else
            val = 0.0;
    }
}

void AudioNormalizer::calculate_thresholds() {
    auto center = (max_ + min_) / 2.0f;
    auto range = (max_ - min_) / 2.0f;

    // 10% off center force either +/-1.0f.
    // Higher == larger dead zone.
    // Lower == more false positives.
    auto threshold = range * 0.1;
    t_hi_ = center + threshold;
    t_lo_ = center - threshold;
}

/* FSKRxProcessor ******************************************/

void FSKRxProcessor::clear_data_bits() {
    data = 0;
    bit_count = 0;
}

void FSKRxProcessor::handle_sync(bool inverted) {
    clear_data_bits();
    has_sync_ = true;
    inverted = inverted;
    word_count = 0;
}

void FSKRxProcessor::process_bits(const buffer_c8_t& buffer) {
    // Process all of the bits in the bits queue.
    while (buffer.count > 0) {
        // Wait until data_ is full.
        if (bit_count < data_bit_count)
            continue;

        // Wait for the sync frame.
        if (!has_sync_) {
            if (diff_bit_count(data, sync_codeword) <= 2)
                handle_sync(/*inverted=*/false);
            else if (diff_bit_count(data, ~sync_codeword) <= 2)
                handle_sync(/*inverted=*/true);
            continue;
        }
    }
}

/* FSKRxProcessor ***************************************/

FSKRxProcessor::FSKRxProcessor() {
}

void FSKRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        return;
    }

    // Decimate by current decim 0 and decim 1.
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    feed_channel_stats(decim_1_out);

    spectrum_samples += decim_1_out.count;

    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        channel_spectrum.feed(decim_1_out, channel_filter_low_f,
                              channel_filter_high_f, channel_filter_transition);
    }

    // process_bits();

    // Update the status.
    samples_processed += buffer.count;

    if (samples_processed >= stat_update_threshold) {
        // send_packet(data);
        samples_processed -= stat_update_threshold;
    }
}

void FSKRxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::FSKRxConfigure:
            configure(*reinterpret_cast<const FSKRxConfigureMessage*>(message));
            break;
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

void FSKRxProcessor::configure(const FSKRxConfigureMessage& message) {
    // Extract message variables.
    deviation = message.deviation;
    channel_decimation = message.channel_decimation;
    // channel_filter_taps = message.channel_filter;

    channel_spectrum.set_decimation_factor(1);
}

void FSKRxProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

void FSKRxProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    const auto sample_rate = message.sample_rate;

    // The actual sample rate is the requested rate * the oversample rate.
    // See oversample.hpp for more details on oversampling.
    baseband_fs = sample_rate * toUType(message.oversample_rate);
    baseband_thread.set_sampling_rate(baseband_fs);

    // TODO: Do we need to use the taps that the decimators get configured with?
    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * sample_rate;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * sample_rate;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * sample_rate;

    // Compute the scalar that corrects the oversample_rate to be x8 when computing
    // the spectrum update interval. The original implementation only supported x8.
    // TODO: Why is this needed here but not in proc_replay? There must be some other
    // assumption about x8 oversampling in some component that makes this necessary.
    const auto oversample_correction = toUType(message.oversample_rate) / 8.0;

    // The spectrum update interval controls how often the waterfall is fed new samples.
    spectrum_interval_samples = sample_rate / (spectrum_rate_hz * oversample_correction);
    spectrum_samples = 0;

    // For high sample rates, the M4 is busy collecting samples so the
    // waterfall runs slower. Reduce the update interval so it runs faster.
    // NB: Trade off: looks nicer, but more frequent updates == more CPU.
    if (sample_rate >= 1'500'000)
        spectrum_interval_samples /= (sample_rate / 750'000);

    switch (message.oversample_rate) {
        case OversampleRate::x4:
            // M4 can't handle 2 decimation passes for sample rates needing x4.
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps);
            decim_1.set<NoopDecim>();
            break;

        case OversampleRate::x8:
            // M4 can't handle 2 decimation passes for sample rates <= 600k.
            if (message.sample_rate < 600'000) {
                decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps);
                decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);
            } else {
                // Using 180k taps to provide better filtering with a single pass.
                decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_180k_wfm_decim_0.taps);
                decim_1.set<NoopDecim>();
            }
            break;

        case OversampleRate::x16:
            decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps);
            decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);
            break;

        case OversampleRate::x32:
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps);
            break;

        case OversampleRate::x64:
            decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps);
            break;

        default:
            chDbgPanic("Unhandled OversampleRate");
            break;
    }

    // Update demodulator based on new decimation. Todo: Confirm this works.
    size_t decim_0_input_fs = baseband_fs;
    size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor();

    size_t decim_1_input_fs = decim_0_output_fs;
    size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor();

    // size_t channel_filter_input_fs = decim_1_output_fs;
    // size_t channel_filter_output_fs = channel_filter_input_fs / channel_decimation;

    size_t demod_input_fs = decim_1_output_fs;

    send_packet((uint32_t)demod_input_fs);

    // Set ready to process data.
    configured = true;
}

void FSKRxProcessor::flush() {
    // word_extractor.flush();
}

void FSKRxProcessor::reset() {
    clear_data_bits();
    has_sync_ = false;
    inverted = false;
    word_count = 0;

    samples_processed = 0;
}

void FSKRxProcessor::send_packet(uint32_t data) {
    data_message.is_data = true;
    data_message.value = data;
    shared_memory.application_queue.push(data_message);
}

/* main **************************************************/

int main() {
    EventDispatcher event_dispatcher{std::make_unique<FSKRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
