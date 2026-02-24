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
#include "dsp_decimate.hpp"

#include "event_m4.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

#include "mathdef.hpp"

float FSKRxProcessor::detect_peak_power(const buffer_c8_t& buffer, int N) {
    int32_t power = 0;

    // Initial window power
    for (int i = 0; i < N; i++) {
        int16_t i_sample = buffer.p[i].real();
        int16_t q_sample = buffer.p[i].imag();
        power += i_sample * i_sample + q_sample * q_sample;
    }

    power = power / N;

    // Convert to dB over noise floor
    float power_db = 10.0f * log10f((float)power / noise_floor);

    // If too weak, treat as no signal
    if (power_db <= 0.0f) return 0;

    return power_db;
}

void FSKRxProcessor::agc_correct_iq(const buffer_c8_t& buffer, int N, float measured_power) {
    float power_db = 10.0f * log10f(measured_power / noise_floor);
    float error_db = target_power_db - power_db;

    if (error_db <= 0) {
        return;
    }

    float gain_scalar = powf(10.0f, error_db / 20.0f);

    for (int i = 0; i < N; i++) {
        buffer.p[i] = {(int8_t)(buffer.p[i].real() * gain_scalar), (int8_t)(buffer.p[i].imag() * gain_scalar)};
    }
}

float FSKRxProcessor::get_phase_diff(const complex16_t& sample0, const complex16_t& sample1) {
    // Calculate the phase difference between two samples.
    float dI = sample1.real() * sample0.real() + sample1.imag() * sample0.imag();
    float dQ = sample1.imag() * sample0.real() - sample1.real() * sample0.imag();
    float phase_diff = atan2f(dQ, dI);

    return phase_diff;
}

void FSKRxProcessor::demodulateFSKBits(const buffer_c16_t& decimator_out, int num_demod_byte) {
    for (; packet_index < num_demod_byte; packet_index++) {
        for (; bit_index < 8; bit_index++) {
            if (samples_eaten >= (int)decimator_out.count) {
                return;
            }

            float phaseSum = 0.0f;
            for (int k = 0; k < SAMPLE_PER_SYMBOL - 1; ++k) {
                float phase = get_phase_diff(
                    decimator_out.p[samples_eaten + k],
                    decimator_out.p[samples_eaten + k + 1]);
                phaseSum += phase;
            }

            phaseSum /= (SAMPLE_PER_SYMBOL - 1);
            phaseSum -= frequency_offset;

            bool bitDecision = (phaseSum > 0.0f);

            rb_buf[packet_index] |= (bitDecision << (7 - bit_index));

            samples_eaten += SAMPLE_PER_SYMBOL;
        }

        bit_index = 0;
    }
}

void FSKRxProcessor::resetPreambleTracking() {
    frequency_offset = 0.0f;
    frequency_offset_estimate = 0.0f;
    phase_buffer_index = 0;
    memset(phase_buffer, 0, sizeof(phase_buffer));
}

void FSKRxProcessor::resetBitPacketIndex() {
    packet_index = 0;
    bit_index = 0;
}

void FSKRxProcessor::resetToDefaultState() {
    parseState = Parse_State_Wait_For_Peak;
    peak_timeout = 0;
    fskPacketData.power = 0.0f;
    resetPreambleTracking();
    resetBitPacketIndex();
}

void FSKRxProcessor::handlePreambleState(const buffer_c16_t& decimator_out) {
    const uint32_t validPreamble = DEFAULT_PREAMBLE;
    static uint32_t preambleValue = 0;

    int hit_idx = -1;

    for (; samples_eaten < (int)decimator_out.count; samples_eaten += SAMPLE_PER_SYMBOL) {
        float phaseSum = 0.0f;

        for (int j = 0; j < SAMPLE_PER_SYMBOL - 1; j++) {
            phaseSum += get_phase_diff(decimator_out.p[samples_eaten + j], decimator_out.p[samples_eaten + j + 1]);
        }

        phase_buffer[phase_buffer_index] = phaseSum / (SAMPLE_PER_SYMBOL - 1);
        phase_buffer_index = (phase_buffer_index + 1) % ROLLING_WINDOW;

        bool bitDecision = (phaseSum > 0.0f);
        preambleValue = (preambleValue << 1) | bitDecision;

        int errors = __builtin_popcountl(preambleValue ^ validPreamble) & 0xFFFFFFFF;

        if (errors == 0) {
            hit_idx = samples_eaten + SAMPLE_PER_SYMBOL;
            fskPacketData.syncWord = preambleValue;
            fskPacketData.max_dB = max_dB;

            for (int k = 0; k < ROLLING_WINDOW; k++) {
                frequency_offset_estimate += phase_buffer[k];
            }

            frequency_offset = frequency_offset_estimate / ROLLING_WINDOW;

            fskPacketData.frequency_offset_hz = (frequency_offset * demod_input_fs) / (2.0f * M_PI);

            preambleValue = 0;
            break;
        }
    }

    if (hit_idx == -1) {
        samples_eaten = samples_eaten;
        return;
    }

    samples_eaten = hit_idx;
    parseState = Parse_State_Sync;
}

void FSKRxProcessor::handleSyncWordState(const buffer_c16_t& decimator_out) {
    const int syncword_bytes = 4;
    const uint32_t validSyncWord = DEFAULT_SYNC_WORD;

    if ((int)decimator_out.count - samples_eaten <= 0) {
        return;
    }

    demodulateFSKBits(decimator_out, syncword_bytes);

    if (packet_index < syncword_bytes || bit_index != 0) {
        return;
    }

    uint32_t receivedSyncWord = (rb_buf[0] << 24) | (rb_buf[1] << 16) | (rb_buf[2] << 8) | rb_buf[3];

    int errors = __builtin_popcountl(receivedSyncWord ^ validSyncWord) & 0xFFFFFFFF;

    if (errors <= 3) {
        fskPacketData.syncWord = receivedSyncWord;
        parseState = Parse_State_PDU_Payload;
        memset(fskPacketData.data, 0, sizeof(fskPacketData.data));
    } else {
        resetToDefaultState();
    }

    memset(rb_buf, 0, sizeof(rb_buf));
    resetBitPacketIndex();
}

void FSKRxProcessor::handlePDUPayloadState(const buffer_c16_t& decimator_out) {
    if ((int)decimator_out.count - samples_eaten <= 0) {
        return;
    }

    demodulateFSKBits(decimator_out, NUM_DATA_BYTE);

    if (packet_index < NUM_DATA_BYTE || bit_index != 0) {
        return;
    }

    fskPacketData.dataLen = NUM_DATA_BYTE;

    // Copy the decoded bits to the packet data
    for (int i = 0; i < NUM_DATA_BYTE; i++) {
        fskPacketData.data[i] |= rb_buf[i];
    }

    FSKRxPacketMessage data_message{&fskPacketData};
    shared_memory.application_queue.push(data_message);

    memset(rb_buf, 0, sizeof(rb_buf));

    resetToDefaultState();
}

void FSKRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured || parseState == Parse_State_Parsing_Data) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    feed_channel_stats(decim_1_out);

    samples_eaten = 0;

    while ((int)decim_1_out.count - samples_eaten > 0) {
        if ((parseState == Parse_State_Wait_For_Peak) || (parseState == Parse_State_Preamble)) {
            float power = detect_peak_power(buffer, buffer.count);

            if (power) {
                parseState = Parse_State_Preamble;
                agc_power = power;
                fskPacketData.power = power;
            } else {
                break;
            }
        }

        if (agc_power) {
            agc_correct_iq(buffer, buffer.count, agc_power);
        }

        if (parseState == Parse_State_Preamble) {
            peak_timeout++;

            // 960,000 fs / 2048 samples = 468.75 Hz, so 55 calls is about 0.053 seconds before timeout.
            if (peak_timeout == 4) {
                resetToDefaultState();
            } else {
                handlePreambleState(decim_1_out);
            }
        }

        if (parseState == Parse_State_Sync) {
            handleSyncWordState(decim_1_out);
        }

        if (parseState == Parse_State_PDU_Payload) {
            handlePDUPayloadState(decim_1_out);
        }
    }
}

void FSKRxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::FSKRxConfigure:
            configure(*reinterpret_cast<const FSKRxConfigureMessage*>(message));
            break;
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            // channel_spectrum.on_message(message);
            break;

        case Message::ID::SampleRateConfig:
            sample_rate_config(*reinterpret_cast<const SampleRateConfigMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            // capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void FSKRxProcessor::configure(const FSKRxConfigureMessage& message) {
    SAMPLE_PER_SYMBOL = message.samplesPerSymbol;
    DEFAULT_SYNC_WORD = message.syncWord;
    NUM_SYNC_WORD_BYTE = message.syncWordLength;
    DEFAULT_PREAMBLE = message.preamble;
    NUM_PREAMBLE_BYTE = message.preambleLength;
    NUM_DATA_BYTE = message.numDataBytes;
}

void FSKRxProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    const auto sample_rate = message.sample_rate;

    baseband_fs = sample_rate * toUType(message.oversample_rate);
    baseband_thread.set_sampling_rate(baseband_fs);

    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * sample_rate;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * sample_rate;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * sample_rate;

    const auto oversample_correction = toUType(message.oversample_rate) / 8.0;

    // The spectrum update interval controls how often the waterfall is fed new samples.
    spectrum_interval_samples = sample_rate / (spectrum_rate_hz * oversample_correction);
    spectrum_samples = 0;

    if (sample_rate >= 1'500'000)
        spectrum_interval_samples /= (sample_rate / 750'000);

    switch (message.oversample_rate) {
        case OversampleRate::x4:
            // M4 can't handle 2 decimation passes for sample rates needing x4.
            decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps);
            decim_1.set<NoopDecim>();
            break;

        case OversampleRate::x8:
            // M4 can't handle 2 decimation passes for sample rates <= 600k.
            if (message.sample_rate < 600'000) {
                decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps);
                decim_1.set<dsp::decimate::FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);
            } else {
                // Using 180k taps to provide better filtering with a single pass.
                decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim8>().configure(taps_180k_wfm_decim_0.taps);
                decim_1.set<NoopDecim>();
            }
            break;

        case OversampleRate::x16:
            decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps);
            decim_1.set<dsp::decimate::FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);
            break;

        case OversampleRate::x32:
            decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps);
            decim_1.set<dsp::decimate::FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps);
            break;

        case OversampleRate::x64:
            decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps);
            decim_1.set<dsp::decimate::FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps);
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

    demod_input_fs = decim_1_output_fs;

    // Set ready to process data.
    configured = true;
}

/* main **************************************************/

int main() {
    EventDispatcher event_dispatcher{std::make_unique<FSKRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
