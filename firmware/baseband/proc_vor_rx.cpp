/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2026 PortaPack Mayhem
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

#include "proc_vor_rx.hpp"

#include "audio_output.hpp"
#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

VorRx::VorRx() {
    channel_spectrum.set_decimation_factor(1);

    baseband_thread.start();
    rssi_thread.start();
}

void VorRx::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        return;
    }

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    channel_spectrum.feed(decim_1_out, channel_filter_low_f, channel_filter_high_f, channel_filter_transition);

    const auto decim_2_out = decim_2.execute(decim_1_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_2_out, dst_buffer);

    feed_channel_stats(channel_out);

    auto audio = demod_am.execute(channel_out, audio_buffer);
    if (vor_enabled) {
        process_vor_metrics(audio);
    }
    audio_compressor.execute_in_place(audio);
    audio_output.write(audio);
}

void VorRx::on_message(const Message* const message) {
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

        default:
            break;
    }
}

void VorRx::configure(const AMConfigureMessage& message) {
    decim_0.configure(message.decim_0_filter.taps);
    decim_1.configure(message.decim_1_filter.taps);
    decim_2.configure(message.decim_2_filter.taps, decim_2_decimation_factor);
    channel_filter.configure(message.channel_filter.taps, channel_filter_decimation_factor);

    constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor;
    constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor;
    constexpr size_t decim_2_output_fs = decim_1_output_fs / decim_2_decimation_factor;
    constexpr size_t channel_filter_input_fs = decim_2_output_fs;

    channel_filter_low_f = message.channel_filter.low_frequency_normalized * channel_filter_input_fs;
    channel_filter_high_f = message.channel_filter.high_frequency_normalized * channel_filter_input_fs;
    channel_filter_transition = message.channel_filter.transition_normalized * channel_filter_input_fs;

    channel_spectrum.set_decimation_factor(message.channel_spectrum_decimation_factor);
    audio_output.configure(message.audio_hpf_lpf_config);

    configured = true;
}

void VorRx::PhaseOscillator::configure(float frequency_hz, float sample_rate_hz) {
    const float step = 2.0f * kPi * frequency_hz / sample_rate_hz;
    step_sin = sinf(step);
    step_cos = cosf(step);
    sin_v = 0.0f;
    cos_v = 1.0f;
}

void VorRx::PhaseOscillator::advance() {
    const float next_cos = (cos_v * step_cos) - (sin_v * step_sin);
    const float next_sin = (sin_v * step_cos) + (cos_v * step_sin);
    cos_v = next_cos;
    sin_v = next_sin;
}

void VorRx::configure_vor(const VorRxConfigureMessage& message) {
    vor_enabled = message.enabled;
    vor_reference_osc.configure(vor_reference_hz, 48000.0f);
    vor_subcarrier_osc.configure(vor_subcarrier_hz, 48000.0f);
    vor_ref_i = 0.0f;
    vor_ref_q = 0.0f;
    vor_var_i = 0.0f;
    vor_var_q = 0.0f;
    vor_sample_count = 0;
}

uint16_t VorRx::normalize_degrees(float degrees) {
    while (degrees < 0.0f) {
        degrees += 360.0f;
    }
    while (degrees >= 360.0f) {
        degrees -= 360.0f;
    }
    return static_cast<uint16_t>(degrees + 0.5f);
}

void VorRx::send_vor_status(uint16_t phase_deg, uint16_t radial_deg, uint16_t ref_level, uint16_t var_level, uint8_t quality, bool valid, bool to_from) {
    const VorRxStatusDataMessage message{phase_deg, radial_deg, ref_level, var_level, quality, valid, to_from};
    shared_memory.application_queue.push(message);
}

void VorRx::process_vor_metrics(const buffer_f32_t& audio) {
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

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<VorRx>()};
    event_dispatcher.run();
    return 0;
}
