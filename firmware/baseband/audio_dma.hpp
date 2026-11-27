/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __AUDIO_DMA_H__
#define __AUDIO_DMA_H__

#include <cstdint>

#include "buffer.hpp"

namespace audio {

struct sample_t {
    union {
        struct {
            int16_t left;
            int16_t right;
        };
        uint32_t raw;
    };
};

using buffer_t = buffer_t<sample_t>;

namespace dma {

void init_audio_in();
void init_audio_out();
void disable();
void shrink_tx_buffer(bool shrink);
void beep_start(uint32_t freq, uint32_t sample_rate, uint32_t beep_duration_ms);
void beep_stop();

audio::buffer_t tx_empty_buffer();
audio::buffer_t rx_empty_buffer();

#ifdef BASEBAND_dab_rx
/* Let the codec pull: the transfer-complete interrupt calls `refill` for every
 * TX transfer freed since the last call, oldest first, so each is written once
 * and in order whatever the processor does. Interrupt context: keep it short;
 * nullptr turns it off. */
using tx_refill_t = void (*)(sample_t* p, size_t count);
void set_tx_refill(tx_refill_t refill);

/* The transfer that has just finished, i.e. the newest writable one, when the
 * DMA's next LLI is `next_index`. Same arithmetic as tx_empty_buffer(). */
constexpr size_t tx_newest_free(size_t next_index, size_t mask) {
    return (next_index + mask - 1) & mask;
}

/* Transfers to refill, ending at tx_newest_free(), after `last_refilled`. Only
 * next + 1 and next + 2 are writable (next may be in flight), so at most two,
 * and a missed interrupt replays stale transfers, never a torn one. Modulo
 * four, four missed completions (~2.7 ms masked) read as none. */
constexpr size_t tx_refill_count(size_t next_index, size_t last_refilled, size_t mask) {
    const size_t behind = (tx_newest_free(next_index, mask) - last_refilled) & mask;
    return behind > 2 ? 2 : behind;
}
#endif

} /* namespace dma */
} /* namespace audio */

#endif /*__AUDIO_DMA_H__*/
