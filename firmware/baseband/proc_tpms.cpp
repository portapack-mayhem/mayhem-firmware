/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
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

#include "proc_tpms.hpp"
#include "audio_dma.hpp"

#include "dsp_fir_taps.hpp"

#include "event_m4.hpp"

TPMSProcessor::TPMSProcessor() {
    decim_0.configure(taps_200k_decim_0.taps);
    decim_1.configure(taps_200k_decim_1.taps);
    baseband_thread.start();
}

void TPMSProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decimator_out = decim_1.execute(decim_0_out, dst_buffer);

    /* 307.2kHz, 256 samples */
    feed_channel_stats(decimator_out);

    for (size_t i = 0; i < decimator_out.count; i++) {
        if (mf_38k4_1t_19k2.execute_once(decimator_out.p[i])) {
            clock_recovery_fsk_19k2(mf_38k4_1t_19k2.get_output());
        }
    }

    for (size_t i = 0; i < decimator_out.count; i += channel_decimation) {
        const auto sliced = ook_slicer_5sps(decimator_out.p[i]);
        slicer_history = (slicer_history << 1) | sliced;

        clock_recovery_ook_8k192(slicer_history, [this](const bool symbol) {
            this->packet_builder_ook_8k192_schrader.execute(symbol);
        });
        clock_recovery_ook_8k4(slicer_history, [this](const bool symbol) {
            this->packet_builder_ook_8k4_schrader.execute(symbol);
        });
    }
}

void TPMSProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::AudioBeep)
        on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(msg));
}

void TPMSProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<TPMSProcessor>()};
    event_dispatcher.run();
    return 0;
}
