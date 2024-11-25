/*
 * Copyright (C) 2024 HTotoo
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

#include "proc_ook_stream_tx.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include "utility.hpp"

OOKProcessorStreamed::OOKProcessorStreamed() {
    configured = false;
    baseband_thread.start();
}

inline void OOKProcessorStreamed::write_sample(const buffer_c8_t& buffer, bool bit_value, size_t i) {
    int8_t re, im;

    if (bit_value) {
        phase = (phase + 200);  // What ?
        sphase = phase + (64 << 18);
        re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
        im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
    } else {
        re = 0;
        im = 0;
    }
    buffer.p[i] = {re, im};
}

void OOKProcessorStreamed::execute(const buffer_c8_t& buffer) {
    if (!configured || !stream) return;

    for (size_t i = 0; i < buffer.count; i++) {
        if (rem_samples <= curr_samples) {
            // get a new sample from stream
            int32_t sample = -13346;
            rem_samples = 0;  // reset my pointer
            curr_samples = 0;
            size_t readed = 0;
            if (configured) readed = stream->read(&sample, 4);  // read from stream // todo htotoo error handling?!
            if (readed == 0) {
                txprogress_message.progress = -10;
                shared_memory.application_queue.push(txprogress_message);  // debug
            } else {
                if (sample == endsignals[readerrs + 1]) {  // if no more samples, stop
                    readerrs++;
                    if (readerrs == 2) {
                        configured = false;
                        txprogress_message.done = true;
                        txprogress_message.progress = 100;
                        curr_hilow = false;
                        shared_memory.application_queue.push(txprogress_message);
                    }
                } else {
                    if (sample < 0) {
                        rem_samples = OOK_SAMPLERATE * ((-1 * sample) / 1000000.0);
                        curr_hilow = false;
                    } else {
                        rem_samples = OOK_SAMPLERATE * (sample / 1000000.0);
                        curr_hilow = true;
                    }
                }
            }
        }
        ++curr_samples;
        write_sample(buffer, curr_hilow, i);
    }
}

void OOKProcessorStreamed::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::ReplayConfig:
            configured = false;
            replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
            break;

        case Message::ID::FIFOData:
            configured = true;
            txprogress_message.progress = -4;
            shared_memory.application_queue.push(txprogress_message);
            break;

        default:
            break;
    }
}

void OOKProcessorStreamed::replay_config(const ReplayConfigMessage& message) {
    if (message.config) {
        txprogress_message.progress = -2;
        shared_memory.application_queue.push(txprogress_message);
        stream = std::make_unique<StreamOutput>(message.config);
        // Tell application that the buffers and FIFO pointers are ready, prefill
        RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};
        shared_memory.application_queue.push(sig_message);
    } else {
        txprogress_message.progress = -3;
        shared_memory.application_queue.push(txprogress_message);
        stream.reset();
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<OOKProcessorStreamed>()};
    event_dispatcher.run();
    return 0;
}
