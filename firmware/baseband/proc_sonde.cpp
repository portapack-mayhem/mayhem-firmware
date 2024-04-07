/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 Mark Thompson
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

#include "proc_sonde.hpp"

#include "dsp_fir_taps.hpp"

#include "event_m4.hpp"

#include "audio_dma.hpp"

SondeProcessor::SondeProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);

    baseband_thread.start();
}

void SondeProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto decimator_out = decim_1_out;

    /* 38.4kHz, 32 samples */
    feed_channel_stats(decimator_out);

    for (size_t i = 0; i < decimator_out.count; i++) {
        if (mf.execute_once(decimator_out.p[i])) {
            clock_recovery_fsk_9600(mf.get_output());
            clock_recovery_fsk_4800(mf.get_output());
        }
    }
}

void SondeProcessor::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::RequestSignal:
            on_signal_message(*reinterpret_cast<const RequestSignalMessage*>(msg));
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(msg));
            break;

        case Message::ID::PitchRSSIConfigure:
            on_pitch_rssi_config(*reinterpret_cast<const PitchRSSIConfigureMessage*>(msg));
            break;

        default:
            break;
    }
}

void SondeProcessor::on_signal_message(const RequestSignalMessage& message) {
    if (message.signal == RequestSignalMessage::Signal::RSSIBeepRequest) {
        float rssi_ratio = (float)last_rssi / RSSI_CEILING;
        uint32_t beep_duration = 0;

        if (rssi_ratio <= PROPORTIONAL_BEEP_THRES) {
            beep_duration = BEEP_MIN_DURATION;
        } else if (rssi_ratio < 1) {
            beep_duration = rssi_ratio * BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
        } else {
            beep_duration = BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
        }

        audio::dma::beep_start(beep_freq, DEFAULT_AUDIO_SAMPLE_RATE, beep_duration);
    }
}

void SondeProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

void SondeProcessor::on_pitch_rssi_config(const PitchRSSIConfigureMessage& message) {
    pitch_rssi_enabled = message.enabled;

    beep_freq = message.rssi * RSSI_PITCH_WEIGHT + BEEP_BASE_FREQ;
    last_rssi = message.rssi;
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<SondeProcessor>()};
    event_dispatcher.run();

    return 0;
}
