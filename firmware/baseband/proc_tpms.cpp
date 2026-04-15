/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2025 Speedster04
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

#include "proc_tpms_all.hpp"
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"

TPMSAllProcessor::TPMSAllProcessor() {
    decim_0.configure(taps_200k_decim_0.taps);
    decim_1.configure(taps_200k_decim_1.taps);
    baseband_thread.start();
}

void TPMSAllProcessor::execute(const buffer_c8_t& buffer) {
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decimator_out = decim_1.execute(decim_0_out, dst_buffer);

    feed_channel_stats(decimator_out);

    for (size_t i = 0; i < decimator_out.count; i++) {
        if (mf.execute_once(decimator_out.p[i])) {
            const float mf_out = mf.get_output();
            clock_recovery_19k2(mf_out);
            clock_recovery_bmw(mf_out);
            clock_recovery_jansite(mf_out);
        }
    }

    for (size_t i = 0; i < decimator_out.count; i += channel_decimation) {
        const auto sliced = ook_slicer_5sps(decimator_out.p[i]);
        slicer_history = (slicer_history << 1) | sliced;
        clock_recovery_ook_8k192(slicer_history, [this](const bool symbol) {
            this->pb_ook_8k192.execute(symbol);
        });
        clock_recovery_ook_8k4(slicer_history, [this](const bool symbol) {
            this->pb_ook_8k4.execute(symbol);
        });
    }
}

void TPMSAllProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::AudioBeep)
        on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(msg));
}

void TPMSAllProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<TPMSAllProcessor>()};
    event_dispatcher.run();
    return 0;
}
