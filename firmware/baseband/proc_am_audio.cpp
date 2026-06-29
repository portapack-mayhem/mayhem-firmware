/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_am_audio.hpp"

#include "audio_output.hpp"
#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include <array>
#include <cmath>
#include "dsp_hilbert.hpp"

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

// Phase 2: Constructor to start threads AFTER object is fully initialized
NarrowbandAMAudio::NarrowbandAMAudio() {
    // Initialize members that threads might access
    channel_spectrum.set_decimation_factor(1);

    // Start threads AFTER everything initialized
    baseband_thread.start();
    rssi_thread.start();
}

void NarrowbandAMAudio::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        return;
    }

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    channel_spectrum.feed(decim_1_out, channel_filter_low_f, channel_filter_high_f, channel_filter_transition);

    const auto decim_2_out = decim_2.execute(decim_1_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_2_out, dst_buffer);

    // TODO: Feed channel_stats post-decimation data?
    feed_channel_stats(channel_out);

    auto audio = demodulate(channel_out);  // now 3 AM demodulation types : demod_am, demod_ssb, demod_ssb_fm (for Wefax)
    if (vor_enabled) {
        process_vor_metrics(audio);
    }
    audio_compressor.execute_in_place(audio);
    audio_output.write(audio);
}

buffer_f32_t NarrowbandAMAudio::demodulate(const buffer_c16_t& channel) {
    switch (modulation_ssb) {  // enum class Modulation : int32_t {DSB = 0, SSB = 1, SSB_FM = 2}
        case (int)(AMConfigureMessage::Modulation::DSB):
            return demod_am.execute(channel, audio_buffer);
            break;

        case (int)(AMConfigureMessage::Modulation::SSB):
            return demod_ssb.execute(channel, audio_buffer);
            break;

        case (int)(AMConfigureMessage::Modulation::SSB_FM):  // Added to handle Weather Fax mode.
            // chDbgPanic("case SSB_FM demodulation");                   // Debug.
            return demod_ssb_fm.execute(channel, audio_buffer);  // Calling a derivative of demod_ssb (USB) , but with different FIR taps + FM audio tones demod.
            break;

        // return demod am as a default
        default:
            return demod_am.execute(channel, audio_buffer);
            break;
    }
}

void NarrowbandAMAudio::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(message);
            break;

        case Message::ID::VorRxConfigure:
            configure_vor(*reinterpret_cast<const VorRxConfigureMessage*>(message));
            break;

        case Message::ID::AMConfigure:
            configure(*reinterpret_cast<const AMConfigureMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void NarrowbandAMAudio::configure(const AMConfigureMessage& message) {
    constexpr size_t decim_0_input_fs = baseband_fs;
    constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

    constexpr size_t decim_1_input_fs = decim_0_output_fs;
    constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

    constexpr size_t decim_2_input_fs = decim_1_output_fs;
    constexpr size_t decim_2_output_fs = decim_2_input_fs / decim_2_decimation_factor;

    constexpr size_t channel_filter_input_fs = decim_2_output_fs;
    // const size_t channel_filter_output_fs = channel_filter_input_fs / channel_filter_decimation_factor;

    decim_0.configure(message.decim_0_filter.taps);
    decim_1.configure(message.decim_1_filter.taps);
    const auto vor_decimation_factor = vor_enabled ? size_t{1} : decim_2_decimation_factor;
    decim_2.configure(message.decim_2_filter.taps, vor_decimation_factor);
    channel_filter.configure(message.channel_filter.taps, channel_filter_decimation_factor);
    channel_filter_low_f = message.channel_filter.low_frequency_normalized * channel_filter_input_fs;
    channel_filter_high_f = message.channel_filter.high_frequency_normalized * channel_filter_input_fs;
    channel_filter_transition = message.channel_filter.transition_normalized * channel_filter_input_fs;

    modulation_ssb = (int)message.modulation;  // now sending by message , 3 types of AM demod :   enum class Modulation : int32_t {DSB = 0, SSB = 1, SSB_FM = 2}
    channel_spectrum.set_decimation_factor(message.channel_spectrum_decimation_factor);
    audio_output.configure(message.audio_hpf_lpf_config);  // hpf in all AM demod modes (AM-6K/9K, USB/LSB,DSB), except Wefax (lpf there).

    configured = true;
}

void NarrowbandAMAudio::PhaseOscillator::configure(float frequency_hz, float sample_rate_hz) {
    const float step = 2.0f * kPi * frequency_hz / sample_rate_hz;
    step_sin = sinf(step);
    step_cos = cosf(step);
    sin_v = 0.0f;
    cos_v = 1.0f;
}

void NarrowbandAMAudio::PhaseOscillator::advance() {
    const float next_cos = (cos_v * step_cos) - (sin_v * step_sin);
    const float next_sin = (sin_v * step_cos) + (cos_v * step_sin);
    cos_v = next_cos;
    sin_v = next_sin;
}

void NarrowbandAMAudio::configure_vor(const VorRxConfigureMessage& message) {
    vor_enabled = message.enabled;
    vor_reference_osc.configure(vor_reference_hz, 48000.0f);
    vor_subcarrier_osc.configure(vor_subcarrier_hz, 48000.0f);
    vor_ref_i = 0.0f;
    vor_ref_q = 0.0f;
    vor_var_i = 0.0f;
    vor_var_q = 0.0f;
    vor_sample_count = 0;
}

uint16_t NarrowbandAMAudio::normalize_degrees(float degrees) {
    while (degrees < 0.0f) {
        degrees += 360.0f;
    }
    while (degrees >= 360.0f) {
        degrees -= 360.0f;
    }
    return static_cast<uint16_t>(degrees + 0.5f);
}

void NarrowbandAMAudio::send_vor_status(uint16_t phase_deg, uint16_t radial_deg, uint16_t ref_level, uint16_t var_level, uint8_t quality, bool valid, bool to_from) {
    const VorRxStatusDataMessage message{phase_deg, radial_deg, ref_level, var_level, quality, valid, to_from};
    shared_memory.application_queue.push(message);
}

void NarrowbandAMAudio::process_vor_metrics(const buffer_f32_t& audio) {
    for (size_t i = 0; i < audio.count; ++i) {
        const float sample = audio.p[i];
        const float ref_i_sample = sample * vor_reference_osc.cosine();
        const float ref_q_sample = sample * vor_reference_osc.sine();
        const float var_mix = sample * vor_subcarrier_osc.cosine();
        const float var_i_sample = var_mix * vor_reference_osc.cosine();
        const float var_q_sample = var_mix * vor_reference_osc.sine();

        vor_ref_i += ref_i_sample;
        vor_ref_q += ref_q_sample;
        vor_var_i += var_i_sample;
        vor_var_q += var_q_sample;

        vor_reference_osc.advance();
        vor_subcarrier_osc.advance();

        ++vor_sample_count;
        if (vor_sample_count < vor_window_samples) {
            continue;
        }

        const float reference_phase = atan2f(vor_ref_q, vor_ref_i);
        const float variable_phase = atan2f(vor_var_q, vor_var_i);
        float phase_diff = (variable_phase - reference_phase) * 180.0f / kPi;
        while (phase_diff < 0.0f) {
            phase_diff += 360.0f;
        }
        while (phase_diff >= 360.0f) {
            phase_diff -= 360.0f;
        }

        const auto phase_deg = normalize_degrees(phase_diff);
        const auto radial_deg = phase_deg;
        const auto ref_level = static_cast<uint16_t>(sqrtf((vor_ref_i * vor_ref_i) + (vor_ref_q * vor_ref_q)));
        const auto var_level = static_cast<uint16_t>(sqrtf((vor_var_i * vor_var_i) + (vor_var_q * vor_var_q)));
        const bool valid = (ref_level > 1200) && (var_level > 600);
        const bool to_from = phase_deg < 180;
        const uint8_t quality = valid ? 100 : 0;

        send_vor_status(phase_deg, radial_deg, ref_level, var_level, quality, valid, to_from);

        vor_sample_count = 0;
        vor_ref_i = 0.0f;
        vor_ref_q = 0.0f;
        vor_var_i = 0.0f;
        vor_var_q = 0.0f;
    }
}

void NarrowbandAMAudio::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<NarrowbandAMAudio>()};
    event_dispatcher.run();
    return 0;
}
