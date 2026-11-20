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

#include "proc_subghz_chat.hpp"
#include "portapack_shared_memory.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"

#include <cstring>

namespace {

// 32 taps at 384 kHz, Kaiser (beta 6): flat to 55 kHz, -70 dB from 100 kHz.
constexpr std::array<int16_t, 32> taps_chat_channel{
    3, -46, -81, 69, 311, 150, -557, -853, 309, 1928, 1196, -2527, -4732, 572, 13066, 23961,
    23961, 13066, 572, -4732, -2527, 1196, 1928, 309, -853, -557, 150, 311, 69, -81, -46, 3};

}  // namespace

SubGhzChatProcessor::SubGhzChatProcessor() {
    decim_0.configure(taps_200k_wfm_decim_0.taps);
    decim_1.configure(taps_200k_wfm_decim_1.taps);
    channel_filter.configure(taps_chat_channel, 2);
    demod.configure(demod_fs, 40000);  // headroom for 19 kHz deviation plus a tuning error
    decoder.configure(demod_fs);
    baseband_thread.start();
}

void SubGhzChatProcessor::execute(const buffer_c8_t& buffer) {
    stir_noise(buffer);
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, channel_buffer);
    feed_channel_stats(channel_out);

    const auto f = demod.execute(channel_out, fm_buffer);
    for (size_t i = 0; i < f.count; i++)
        decoder.execute(f.p[i]);
}

// Randomness for the app's encryption (run id, IVs): the low bits of the raw samples, which
// are receiver noise, stirred into the first bytes of bb_data. Nothing else uses bb_data
// while this image runs; sending switches to the FSK image, which takes it for its bits.
void SubGhzChatProcessor::stir_noise(const buffer_c8_t& buffer) {
    uint32_t x = noise_;
    for (size_t i = 0; i < 32 && i < buffer.count; i++)
        x = ((x << 5) | (x >> 27)) ^ (uint8_t)buffer.p[i].real() ^ ((uint32_t)(uint8_t)buffer.p[i].imag() << 16);
    noise_ = x;
    uint8_t* pool = shared_memory.bb_data.data + 4 * (noise_index_++ % (SubGhzChatPacketMessage::NOISE_POOL_BYTES / 4));
    for (int i = 0; i < 4; i++)
        pool[i] ^= x >> (8 * i);
}

void SubGhzChatProcessor::on_packet(void* context, const uint8_t* payload, size_t length) {
    auto& m = static_cast<SubGhzChatProcessor*>(context)->packet_message;
    m.length = length;
    memcpy(m.data, payload, length);
    shared_memory.application_queue.push(m);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SubGhzChatProcessor>()};
    event_dispatcher.run();
    return 0;
}
