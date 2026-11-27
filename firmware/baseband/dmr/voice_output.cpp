/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#include "voice_output.hpp"
#include "voice_gain.hpp"
#include "ambe_fec.h"
#include "vocoder/decoder.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_dma.hpp"
#include "gpdma.hpp"
#include "i2s.hpp"
#include <cstring>

extern "C" {
#include <dmr/voice.h>
#include <dmr/embedded.h>
}

namespace {
static_assert(sizeof(shared_memory.bb_data.dmr_voice) <= sizeof(shared_memory.bb_data.dummy_seperate), "DMR scratch must not enlarge shared memory");
constexpr unsigned frame_samples = 160;
constexpr unsigned audio_block = 32;
constexpr auto& audio_channel = lpc43xx::gpdma::channels[portapack::i2s0_tx_gpdma_channel_number];
alignas(4) uint32_t audio_samples[2][audio_block]{};
std::array<lpc43xx::gpdma::channel::LLI, 2> audio_lli{};
}  // namespace

DmrVoiceOutput* DmrVoiceOutput::instance = nullptr;

DmrVoiceOutput::DmrVoiceOutput() {
    instance = this;
    const lpc43xx::gpdma::channel::Control control{
        .transfersize = audio_block, .sbsize = 4, .dbsize = 4, .swidth = 2, .dwidth = 2, .s = 0, .d = 1, .si = 1, .di = 0, .prot1 = 0, .prot2 = 0, .prot3 = 0, .i = 1};

    for (unsigned i = 0; i < 2; ++i) {
        audio_lli[i].srcaddr = reinterpret_cast<uint32_t>(audio_samples[i]);
        audio_lli[i].destaddr = reinterpret_cast<uint32_t>(&LPC_I2S0->TXFIFO);
        audio_lli[i].lli = reinterpret_cast<uint32_t>(&audio_lli[1 - i]);
        audio_lli[i].control = control;
    }

    const lpc43xx::gpdma::channel::Config config{
        .e = 0, .srcperipheral = 0xa, .destperipheral = 0xa, .flowcntrl = lpc43xx::gpdma::FlowControl::MemoryToPeripheral_DMAControl, .ie = 1, .itc = 1, .l = 0, .a = 0, .h = 0};

    audio_channel.set_handlers(audio_interrupt, audio_error);
    audio_channel.configure(audio_lli[0], config);
    audio_channel.enable();
    // M0 owns codec/amplifier mute, including the navigation mute setting.
}

DmrVoiceOutput::~DmrVoiceOutput() {
    audio_channel.disable();
    lpc43xx::i2s::i2s0::tx_mute();
    instance = nullptr;
}

void DmrVoiceOutput::reset() {
    call.reset();
    silence_samples = 2457600 / 2;
    reset_audio();
}

void DmrVoiceOutput::advance(unsigned samples) {
    if (silence_samples >= 2457600 / 2) return;

    silence_samples += samples;

    if (silence_samples >= 2457600 / 2) reset();
}

void DmrVoiceOutput::reset_audio() {
    chSysLock();
    ++epoch;
    head = tail = 0;
    pcm.reset();
    std::memset(audio_samples, 0, sizeof(audio_samples));
    selected_phase = 0;

    chSysUnlock();
}

bool DmrVoiceOutput::audio_active() {
    chSysLock();
    const bool active = head != tail || pcm.has_audio();
    chSysUnlock();

    return active;
}

void DmrVoiceOutput::burst(const dmr_burst_t& b) {
    const bool voice_sync = b.has_sync && (b.sync_kind == DMR_SYNC_MS_VOICE || b.sync_kind == DMR_SYNC_BS_VOICE);

    if (b.has_sync && !voice_sync) {
        if (selected_phase && selected_phase != b.timeslot) return;

        reset();
        call.header(b);

        if (call.valid) {
            selected_phase = b.timeslot;
            last_sample = b.first_sample_index;
        }
        silence_samples = 0;
        return;
    }

    if (voice_sync && (!selected_phase || b.first_sample_index - last_sample > 2304)) {
        if (selected_phase) call.reset();

        reset_audio();
        selected_phase = b.timeslot;
        call.select(selected_phase);
    }

    if (!selected_phase || selected_phase != b.timeslot) return;
    if (voice_sync) call.boundary();

    if (!b.has_sync) {
        dmr_emb_t emb;

        if (!dmr_emb_decode(&b, &emb)) {
            call.boundary();
            return;
        }

        if (emb.privacy) {
            reset();
            return;
        }

        call.embedded(emb);
    }

    silence_samples = 0;
    last_sample = b.first_sample_index;
    uint8_t payload[27];

    if (!dmr_voice_burst_extract(&b, payload)) return;

    chSysLock();

    if (head - tail <= 6) {
        for (unsigned i = 0; i < 3; ++i)
            std::memcpy(queue[(head++) % 9].data, payload + i * 9, 9);
    }

    chSysUnlock();
    EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
}

void DmrVoiceOutput::service() {
    while (true) {
        Frame frame;
        chSysLock();

        if (head == tail || pcm.free() < frame_samples) {
            chSysUnlock();
            return;
        }

        const uint32_t generation = epoch;

        frame = queue[(tail++) % 9];
        chSysUnlock();

        if (decoder_epoch != generation) {
            dmr_vocoder::reset();
            decoder_epoch = generation;
        }

        uint8_t bits[7];
        int16_t samples[frame_samples]{};
        const int corrected = dmr_ambe_decode(frame.data, bits);

        if (corrected >= 0) {
            dmr_vocoder::decode(bits, samples);
            for (auto& sample : samples) sample = dmr_voice_gain(sample);
        } else {
            decoder_epoch = ~0u;
        }

        chSysLock();

        if (generation == epoch) {
            pcm.push(samples, frame_samples);
        }
        chSysUnlock();
    }
}

void DmrVoiceOutput::audio_interrupt() {
    auto& self = *instance;
    // With two descriptors, next LLI identifies the completed buffer.
    const auto index = audio_channel.next_lli() - &audio_lli[0];
    auto* out = audio_samples[index];

    for (unsigned i = 0; i < audio_block; ++i) {
        const int16_t value = self.pcm.sample24k();
        out[i] = static_cast<uint16_t>(value) * 0x10001u;
    }

    if (self.pcm.take_wakeup()) EventDispatcher::events_flag_isr(EVT_MASK_SPECTRUM);
}

void DmrVoiceOutput::audio_error() {
    audio_channel.disable();
}
