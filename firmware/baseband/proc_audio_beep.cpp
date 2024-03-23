/*
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

#include "proc_audio_beep.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"

AudioBeepProcessor::AudioBeepProcessor() {
}

void AudioBeepProcessor::execute(const buffer_c8_t& buffer) {
    (void)buffer;
}

void AudioBeepProcessor::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::RequestSignal:
            on_signal_message(*reinterpret_cast<const RequestSignalMessage*>(msg));
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(msg));
            break;

        default:
            break;
    }
}

void AudioBeepProcessor::on_signal_message(const RequestSignalMessage& message) {
    if (message.signal == RequestSignalMessage::Signal::BeepStopRequest) {
        audio::dma::beep_stop();
    }
}

void AudioBeepProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<AudioBeepProcessor>()};
    event_dispatcher.run();
    return 0;
}
