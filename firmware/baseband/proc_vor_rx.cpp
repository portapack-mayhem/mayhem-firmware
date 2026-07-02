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

#include <algorithm>

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

    // No decim_2 stage: the standard AM decim_2 filter is a ~4.5 kHz low-pass
    // that would attenuate the 9960 Hz VOR subcarrier by ~60 dB and destroy the
    // reference phase. The channel stays at 48 kHz so the subcarrier survives.
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

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
    channel_filter.configure(message.channel_filter.taps, channel_filter_decimation_factor);

    constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor;
    constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor;
    constexpr size_t channel_filter_input_fs = decim_1_output_fs;

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

void VorRx::PhaseOscillator::renormalize() {
    // The recursive rotator slowly drifts in amplitude over long runs;
    // rescale to the unit circle so accumulated tone levels stay meaningful.
    const float mag = sqrtf((cos_v * cos_v) + (sin_v * sin_v));
    if (mag > 0.0f) {
        const float inv = 1.0f / mag;
        cos_v *= inv;
        sin_v *= inv;
    }
}

void VorRx::configure_vor(const VorRxConfigureMessage& message) {
    vor_enabled = message.enabled;
    vor_reference_osc.configure(vor_reference_hz, 48000.0f);
    vor_subcarrier_osc.configure(vor_subcarrier_hz, 48000.0f);
    vor_ref_i = 0.0f;
    vor_ref_q = 0.0f;
    vor_var_i = 0.0f;
    vor_var_q = 0.0f;
    vor_dc_sum = 0.0f;
    vor_sub_lp_i = 0.0f;
    vor_sub_lp_q = 0.0f;
    vor_sub_prev_i = 0.0f;
    vor_sub_prev_q = 0.0f;
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
        const float ref_cos = vor_reference_osc.cosine();
        const float ref_sin = vor_reference_osc.sine();
        const float sub_cos = vor_subcarrier_osc.cosine();
        const float sub_sin = vor_subcarrier_osc.sine();

        // Variable signal: 30 Hz amplitude modulation of the carrier envelope.
        // Correlate the envelope against a local 30 Hz tone (DC rejected because
        // the window spans an exact integer number of 30 Hz cycles).
        vor_dc_sum += sample;
        vor_var_i += sample * ref_cos;
        vor_var_q -= sample * ref_sin;

        // Reference signal: 30 Hz FM on the 9960 Hz subcarrier. Quadrature
        // down-convert the subcarrier, low-pass to its +/-480 Hz baseband, then
        // FM-demodulate (phase difference between successive samples).
        const float sc_i = sample * sub_cos;
        const float sc_q = -sample * sub_sin;
        vor_sub_lp_i += (1.0f - vor_sub_lp_alpha) * (sc_i - vor_sub_lp_i);
        vor_sub_lp_q += (1.0f - vor_sub_lp_alpha) * (sc_q - vor_sub_lp_q);

        const float num = (vor_sub_lp_q * vor_sub_prev_i) - (vor_sub_lp_i * vor_sub_prev_q);
        const float den = (vor_sub_lp_i * vor_sub_prev_i) + (vor_sub_lp_q * vor_sub_prev_q);
        const float fm = atan2f(num, den);
        vor_sub_prev_i = vor_sub_lp_i;
        vor_sub_prev_q = vor_sub_lp_q;

        vor_ref_i += fm * ref_cos;
        vor_ref_q -= fm * ref_sin;

        vor_reference_osc.advance();
        vor_subcarrier_osc.advance();

        ++vor_sample_count;
        if (vor_sample_count < vor_window_samples) {
            continue;
        }

        const float inv_n = 1.0f / static_cast<float>(vor_window_samples);
        const float variable_phase = atan2f(vor_var_q, vor_var_i);
        const float reference_phase = atan2f(vor_ref_q, vor_ref_i);
        // Real VOR radials increase clockwise, i.e. the variable tone lags the
        // reference by the bearing. The decoder therefore reports
        // (reference - variable); the opposite order mirrors every bearing to
        // 360 - true. Add back the receive-chain phase lag (see
        // vor_ref_phase_lag_deg): with this subtraction order the delayed
        // reference makes the raw radial read low, so the correction is added.
        float phase_diff = ((reference_phase - variable_phase) * 180.0f / kPi) + vor_ref_phase_lag_deg;
        while (phase_diff < 0.0f) {
            phase_diff += 360.0f;
        }
        while (phase_diff >= 360.0f) {
            phase_diff -= 360.0f;
        }

        // Scale-invariant lock metrics (independent of RF/AGC level):
        //  - var_ratio: 30 Hz AM depth vs. carrier DC (~0.30 for a real VOR).
        //  - ref_amp:   FM-demodulated 30 Hz amplitude (phase-only, so it does
        //               not depend on signal amplitude, ~0.065 when locked).
        const float dc_level = vor_dc_sum * inv_n;
        const float var_amp = 2.0f * sqrtf((vor_var_i * vor_var_i) + (vor_var_q * vor_var_q)) * inv_n;
        const float ref_amp = 2.0f * sqrtf((vor_ref_i * vor_ref_i) + (vor_ref_q * vor_ref_q)) * inv_n;
        const float var_ratio = (dc_level > 0.0f) ? (var_amp / dc_level) : 0.0f;

        const auto phase_deg = normalize_degrees(phase_diff);
        const auto radial_deg = phase_deg;
        const auto ref_level = static_cast<uint16_t>(std::min(65535.0f, ref_amp * 10000.0f));
        const auto var_level = static_cast<uint16_t>(std::min(65535.0f, var_ratio * 1000.0f));
        const bool valid = (ref_amp > 0.03f) && (var_ratio > 0.15f);
        // TO/FROM depends on the operator-selected OBS course, which only the
        // application knows, so it is derived there (see VorRxView). Emit a
        // stable placeholder here rather than a radial-only guess.
        const bool to_from = false;
        uint8_t quality = 0;
        if (valid) {
            const float q = (ref_amp / 0.065f) * 100.0f;
            quality = static_cast<uint8_t>(std::min(100.0f, std::max(0.0f, q)));
        }

        send_vor_status(phase_deg, radial_deg, ref_level, var_level, quality, valid, to_from);

        vor_reference_osc.renormalize();
        vor_subcarrier_osc.renormalize();
        vor_sample_count = 0;
        vor_ref_i = 0.0f;
        vor_ref_q = 0.0f;
        vor_var_i = 0.0f;
        vor_var_q = 0.0f;
        vor_dc_sum = 0.0f;
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<VorRx>()};
    event_dispatcher.run();
    return 0;
}
