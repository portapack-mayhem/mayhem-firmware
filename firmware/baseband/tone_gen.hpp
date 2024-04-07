/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __TONE_GEN_H__
#define __TONE_GEN_H__

#include <cstdint>
#include <cstddef>

class ToneGen {
   public:
    void configure(const uint32_t delta, const float tone_mix_weight);
    int32_t process(const int32_t sample_in);

    void configure_beep(const uint32_t freq, const uint32_t sample_rate);
    int16_t process_beep();

   private:
    float input_mix_weight_{1};
    float tone_mix_weight_{0};

    float f_delta_{0.0};
    float f_tone_phase_{0.0};

    uint32_t delta_{0};
    uint32_t tone_phase_{0};
};

#endif /* __TONE_GEN_H__ */
