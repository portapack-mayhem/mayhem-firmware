/*
 * Copyright (C) 2026 OE1LBS
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

#include "proc_horus_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"

#include <cstring>

HorusRxProcessor::HorusRxProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);
    audio_output.configure(false);

    configured = true;
}

void HorusRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    feed_channel_stats(channel_out);

    process_channel(channel_out);

    // One bounded slice of the spectrum / tone estimation per buffer, so no single
    // buffer carries the whole FFT.
    demod.background_step();

    // USB audio, so the tones can be heard (and fed to another decoder if wanted).
    auto audio_out = demod_ssb.execute(channel_out, audio_buffer);
    audio_output.write(audio_out);
}

void HorusRxProcessor::process_channel(const buffer_c16_t& channel) {
    constexpr float scale = 1.0f / 32768.0f;
    for (size_t i = 0; i < channel.count; i++) {
        pair_acc.re += channel.p[i].real() * scale;
        pair_acc.im += channel.p[i].imag() * scale;
        pair_phase = !pair_phase;
        if (pair_phase) continue;

        const horus::cf32 x{0.5f * pair_acc.re, 0.5f * pair_acc.im};
        pair_acc = {0.0f, 0.0f};

        const int sym = demod.process(x);
        if (sym >= 0) {
            if (framer.push_symbol((uint8_t)sym)) {
                mirrored = false;
                send_packet(framer.packet());
            }
            if (framer_mirrored.push_symbol((uint8_t)(3 - sym))) {
                mirrored = true;
                send_packet(framer_mirrored.packet());
            }
        }

        if (--status_countdown == 0) {
            status_countdown = status_interval;
            send_status();
        }
    }
}

void HorusRxProcessor::send_packet(const horus::Packet& p) {
    const auto& st = demod.status();

    HorusPacketMessage msg{};
    memcpy(msg.data, p.data, p.size);
    msg.size = p.size;
    msg.version = p.version;
    msg.bits_corrected = p.bits_corrected;
    msg.snr_x10 = (int16_t)(st.snr_db * 10.0f);
    float centre = 0.0f;
    for (int k = 0; k < horus::kTones; k++) centre += st.tone_hz[k];
    centre /= horus::kTones;
    msg.freq_hz = (int16_t)(mirrored ? -centre : centre);
    shared_memory.application_queue.push(msg);
}

void HorusRxProcessor::send_status() {
    const auto& st = demod.status();
    HorusRxStatusMessage msg{};
    // report the tones as they are on air: lowest first, mirrored back if needed
    for (int k = 0; k < horus::kTones; k++)
        msg.tone_hz[k] = (int16_t)(mirrored ? -st.tone_hz[horus::kTones - 1 - k] : st.tone_hz[k]);
    msg.snr_x10 = (int16_t)(st.snr_db * 10.0f);
    msg.signal_x10 = (int16_t)(st.signal_db * 10.0f);
    msg.tones_valid = st.tones_valid;

    shared_memory.application_queue.push(msg);
}

void HorusRxProcessor::on_message(const Message* const) {
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<HorusRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
