/*
 * Copyright (C) 2026 Xmoo26
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

#include "proc_vdl2.hpp"
#include "portapack_shared_memory.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"

#include <cstring>

namespace {

// 32 taps at 48 kHz, Kaiser (beta 6): -1 dB at 10.5 kHz, -70 dB from 16 kHz.
constexpr std::array<int16_t, 32> taps_vdl2_channel{
    -14, -38, 76, 135, -220, -339, 500, 714, -998, -1374, 1881, 2591, -3660, -5493, 9585, 29421,
    29421, 9585, -5493, -3660, 2591, 1881, -1374, -998, 714, 500, -339, -220, 135, 76, -38, -14};

}  // namespace

VDL2Processor::VDL2Processor() {
    decim_0.configure(taps_16k0_decim_0.taps);
    decim_1.configure(taps_16k0_decim_1.taps);
    channel_filter.configure(taps_vdl2_channel, 1);
    baseband_thread.start();
}

void VDL2Processor::execute(const buffer_c8_t& buffer) {
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, channel_buffer);
    feed_channel_stats(channel_out);

    for (size_t i = 0; i < channel_out.count; i++)
        demod.execute({(float)channel_out.p[i].real(), (float)channel_out.p[i].imag()});
}

void VDL2Processor::on_frame(void* context, const uint8_t* frame, size_t length) {
    auto& self = *static_cast<VDL2Processor*>(context);
    auto& m = self.frame_message;
    m.length = length;
    m.offset_hz = (int16_t)self.demod.last_offset_hz();
    memcpy(m.data, frame, (length < VDL2FrameMessage::MAX_LENGTH) ? length : VDL2FrameMessage::MAX_LENGTH);
    shared_memory.application_queue.push(m);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<VDL2Processor>()};
    event_dispatcher.run();
    return 0;
}
