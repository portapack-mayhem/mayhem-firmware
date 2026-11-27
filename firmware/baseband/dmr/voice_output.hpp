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

#ifndef DMR_VOICE_OUTPUT_HPP
#define DMR_VOICE_OUTPUT_HPP

#include "receiver.h"
#include "voice_pcm.hpp"
#include "call_info.hpp"
#include "portapack_shared_memory.hpp"
#include <cstdint>

class DmrVoiceOutput {
   public:
    DmrVoiceOutput();
    ~DmrVoiceOutput();
    void reset();  // RX thread: cancel queued audio without touching codec state.
    void burst(const dmr_burst_t& burst);
    void service();                  // Event-loop thread: preemptible FEC and vocoder work.
    void advance(unsigned samples);  // RX samples at 2.4576 MHz.
    bool audio_active();             // RX thread: queued frames, PCM or interpolation tail.
    DmrCallInfo call{};              // Read on the RX thread only.

   private:
    static DmrVoiceOutput* instance;
    static void audio_interrupt();
    static void audio_error();
    void reset_audio();

    struct Frame {
        uint8_t data[9];
    } queue[9]{};

    unsigned head = 0, tail = 0;
    uint32_t epoch = 0, decoder_epoch = ~0u;
    DmrVoicePcm pcm{shared_memory.bb_data.dmr_voice.pcm};
    unsigned selected_phase = 0;
    uint64_t last_sample = 0;
    unsigned silence_samples = 2457600 / 2;
};
#endif
