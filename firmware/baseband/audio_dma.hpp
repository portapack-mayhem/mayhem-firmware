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

} /* namespace dma */
} /* namespace audio */

#endif /*__AUDIO_DMA_H__*/
