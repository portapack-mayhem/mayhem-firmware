/*
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

#include "proc_lrpt_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"

#include <cstring>

// In .bss rather than in the processor, so the link checks that it fits the M4 RAM.
static lrpt::Shared shared;

LrptRxProcessor::LrptRxProcessor() {
    // fs=2304000: flat to 67 kHz, the channel is 54 kHz and the Doppler and the tuning error
    // a few kHz; what folds back onto it is down 43 dB.
    decim_0.configure(taps_25k0_tetra_decim_0.taps);
    baseband_thread.start();
}

void LrptRxProcessor::execute(const buffer_c8_t& buffer) {
    const auto out = decim_0.execute(buffer, dst_buffer);
    feed_channel_stats(out);

    for (size_t i = 0; i < out.count; i++)
        demod.execute({(float)out.p[i].real(), (float)out.p[i].imag()});

    status_count_ += out.count;
    if (status_count_ >= status_samples) {
        status_count_ -= status_samples;
        send_status();
    }
}

void LrptRxProcessor::send_status() {
    demod.constellation(&shared.constellation[0][0]);
    auto& m = status_message_;
    m.shared = &shared;
    m.offset_hz = demod.frequency_offset_hz();
    m.frames = frames_;
    m.overruns = overruns_;
    m.ber_permille = demod.ber_permille();
    m.flags = (demod.carrier_locked() ? LrptRxStatusMessage::FLAG_CARRIER : 0) |
              (demod.code_locked() ? LrptRxStatusMessage::FLAG_CODE : 0) |
              (demod.frame_synced() ? LrptRxStatusMessage::FLAG_SYNC : 0);
    shared_memory.application_queue.push(m);
}

// Never waits for the M0: a frame for a slot still in use is dropped.
void LrptRxProcessor::on_frame(void* context, const uint8_t* frame) {
    auto& self = *static_cast<LrptRxProcessor*>(context);
    self.frames_++;
    const uint8_t slot = self.next_slot_;
    if (shared.busy[slot]) {
        self.overruns_++;
        return;
    }
    memcpy(shared.frames[slot], frame, lrpt::FRAME_BYTES);
    shared.busy[slot] = 1;
    const LrptRxFrameMessage m{&shared, slot};
    if (!shared_memory.application_queue.push(m)) {
        shared.busy[slot] = 0;
        self.overruns_++;
        return;
    }
    self.next_slot_ = (slot + 1) % lrpt::RING_SLOTS;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<LrptRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
