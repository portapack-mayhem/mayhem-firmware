/*
 * Copyright (C) 2026 justin080
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

#include "proc_seewetter.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_dma.hpp"
#include "event_m4.hpp"
#include "dsp_fir_taps.hpp"

SeewetterProcessor::SeewetterProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);  // -> 24 kHz complex
    audio_output.configure(false);                        // tones are AGC'd by the demodulator
}

void SeewetterProcessor::execute(const buffer_c8_t& buffer) {
    if (config_pending.load(std::memory_order_acquire)) {
        const Config c = pending_config;
        config_pending.store(false, std::memory_order_release);
        demod.configure(c.baud_x100, c.shift, c.polarity, c.afc != 0, c.squelch);
        configured = true;
    }
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

    feed_channel_stats(channel_out);

    const size_t count = channel_out.count < audio.size() ? channel_out.count : audio.size();
    for (size_t i = 0; i < count; i++) {
        const seewetter::cf x{(float)channel_out.p[i].real(), (float)channel_out.p[i].imag()};
        audio[i] = demod.process(x);
    }
    audio_output.write(buffer_f32_t{audio.data(), count, channel_out.sampling_rate});

    if (demod.char_count > 0) send_text();
    if (demod.spectrum_ready) send_status();
}

void SeewetterProcessor::send_text() {
    text_message.stopbits = seewetter::KIND_TEXT;
    const size_t n = demod.char_count;
    for (size_t i = 0; i < n; i++) text_message.data[i] = demod.chars[i];
    text_message.data_len = n;
    if (shared_memory.application_queue.push(text_message))
        demod.clear_chars();  // otherwise keep them and retry with the next block
}

void SeewetterProcessor::send_status() {
    status_message.stopbits = seewetter::KIND_STATUS;
    for (size_t i = 0; i < seewetter::SPECTRUM_BINS; i++) status_message.data[i] = demod.spectrum[i];
    status_message.data_len = seewetter::SPECTRUM_BINS;
    status_message.mark_tone = (int16_t)demod.afc_hz;
    status_message.space_tone = (int16_t)demod.target_hz;
    float q = demod.quality * 100.0f;
    if (q < 0.0f) q = 0.0f;
    if (q > 100.0f) q = 100.0f;
    status_message.baud = (uint16_t)q;
    uint16_t flags = 0;
    if (demod.active_pol == seewetter::Demod::POL_INVERTED) flags |= seewetter::FLAG_INVERTED;
    if (demod.locked) flags |= seewetter::FLAG_LOCKED;
    if (demod.squelch_open) flags |= seewetter::FLAG_SQUELCH_OPEN;
    if (pending_config.afc) flags |= seewetter::FLAG_AFC;
    status_message.shift = flags;
    if (shared_memory.application_queue.push(status_message))
        demod.spectrum_ready = false;
}

void SeewetterProcessor::on_message(const Message* const message) {
    if (message->id != Message::ID::RTTYData) return;
    const auto& m = *reinterpret_cast<const RTTYDataMessage*>(message);
    if (m.stopbits != seewetter::KIND_CONFIG || m.data_len < 3) return;

    pending_config.baud_x100 = m.baud;
    pending_config.shift = m.shift;
    pending_config.polarity = m.data[0];
    pending_config.afc = m.data[1];
    pending_config.squelch = m.data[2];
    config_pending.store(true, std::memory_order_release);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<SeewetterProcessor>()};
    event_dispatcher.run();
    return 0;
}
