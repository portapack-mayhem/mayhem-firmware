/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PORTAPACK_PERSISTENT_MEMORY_H__
#define __PORTAPACK_PERSISTENT_MEMORY_H__

#include <cstdint>

#include "rf_path.hpp"

namespace portapack {
namespace persistent_memory {

using ppb_t = int32_t;

rf::Frequency tuned_frequency();
void set_tuned_frequency(const rf::Frequency new_value);

ppb_t correction_ppb();
void set_correction_ppb(const ppb_t new_value);

int32_t afsk_mark_freq();
void set_afsk_mark(const int32_t new_value);

int32_t afsk_space_freq();
void set_afsk_space(const int32_t new_value);

int32_t afsk_bitrate();
void set_afsk_bitrate(const int32_t new_value);

uint8_t afsk_config();
void set_afsk_config(const uint8_t new_value);

bool playing_dead();
void set_playing_dead(const bool new_value);

uint32_t playdead_sequence();
void set_playdead_sequence(const uint32_t new_value);

} /* namespace persistent_memory */
} /* namespace portapack */

#endif/*__PORTAPACK_PERSISTENT_MEMORY_H__*/
