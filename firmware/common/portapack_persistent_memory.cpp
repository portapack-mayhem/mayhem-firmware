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

#include "portapack_persistent_memory.hpp"

#include "portapack.hpp"

#include "hal.h"

#include "utility.hpp"

#include "memory_map.hpp"
using portapack::memory::map::backup_ram;

#include <algorithm>
#include <utility>

namespace portapack {
namespace persistent_memory {

constexpr rf::Frequency tuned_frequency_reset_value { 88000000 };

using ppb_range_t = range_t<ppb_t>;
constexpr ppb_range_t ppb_range { -99000, 99000 };
constexpr ppb_t ppb_reset_value { 0 };

using afsk_freq_range_t = range_t<int32_t>;
constexpr afsk_freq_range_t afsk_freq_range { 1, 400 };
constexpr int32_t afsk_mark_reset_value { 48 };
constexpr int32_t afsk_space_reset_value { 88 };

using afsk_bitrate_range_t = range_t<int32_t>;
constexpr afsk_bitrate_range_t afsk_bitrate_range { 600, 9600 };
constexpr int32_t afsk_bitrate_reset_value { 1200 };

using afsk_bw_range_t = range_t<int32_t>;
constexpr afsk_bw_range_t afsk_bw_range { 1, 40 };
constexpr int32_t afsk_bw_reset_value { 15 };

/* struct must pack the same way on M4 and M0 cores. */
struct data_t {
	// General config
	int64_t tuned_frequency;
	int32_t correction_ppb;
	
	// AFSK modem
	int32_t afsk_mark_freq;
	int32_t afsk_space_freq;		// Todo: reduce size, only 256 bytes of NVRAM !
	int32_t afsk_bitrate;
	int32_t afsk_bw;
	uint32_t afsk_config;
	
	// Play dead unlock
	uint32_t playing_dead;
	uint32_t playdead_sequence;
	
	uint32_t ui_config;
};

static_assert(sizeof(data_t) <= backup_ram.size(), "Persistent memory structure too large for VBAT-maintained region");

static data_t* const data = reinterpret_cast<data_t*>(backup_ram.base());

rf::Frequency tuned_frequency() {
	rf::tuning_range.reset_if_outside(data->tuned_frequency, tuned_frequency_reset_value);
	return data->tuned_frequency;
}

void set_tuned_frequency(const rf::Frequency new_value) {
	data->tuned_frequency = rf::tuning_range.clip(new_value);
}

ppb_t correction_ppb() {
	ppb_range.reset_if_outside(data->correction_ppb, ppb_reset_value);
	return data->correction_ppb;
}

void set_correction_ppb(const ppb_t new_value) {
	const auto clipped_value = ppb_range.clip(new_value);
	data->correction_ppb = clipped_value;
	portapack::clock_manager.set_reference_ppb(clipped_value);
}

int32_t afsk_mark_freq() {
	afsk_freq_range.reset_if_outside(data->afsk_mark_freq, afsk_mark_reset_value);
	return data->afsk_mark_freq;
}

void set_afsk_mark(const int32_t new_value) {
	data->afsk_mark_freq = afsk_freq_range.clip(new_value);
}

int32_t afsk_space_freq() {
	afsk_freq_range.reset_if_outside(data->afsk_space_freq, afsk_space_reset_value);
	return data->afsk_space_freq;
}

void set_afsk_space(const int32_t new_value) {
	data->afsk_space_freq = afsk_freq_range.clip(new_value);
}

int32_t afsk_bitrate() {
	afsk_bitrate_range.reset_if_outside(data->afsk_bitrate, afsk_bitrate_reset_value);
	return data->afsk_bitrate;
}

void set_afsk_bitrate(const int32_t new_value) {
	data->afsk_bitrate = afsk_bitrate_range.clip(new_value);
}

int32_t afsk_bw() {
	afsk_bw_range.reset_if_outside(data->afsk_bw, afsk_bw_reset_value);
	return data->afsk_bw;
}

void set_afsk_bw(const int32_t new_value) {
	data->afsk_bw = afsk_bw_range.clip(new_value);
}

uint32_t afsk_config() {
	return data->afsk_config;
}

uint8_t afsk_format() {
	return ((data->afsk_config >> 16) & 0xFF);
}

uint8_t afsk_repeats() {
	return (data->afsk_config >> 24);
}

void set_afsk_config(const uint32_t new_value) {
	data->afsk_config = new_value;
}

uint32_t playing_dead() {
	return data->playing_dead;
}

void set_playing_dead(const uint32_t new_value) {
	data->playing_dead = new_value;
}

uint32_t playdead_sequence() {
	return data->playdead_sequence;
}

void set_playdead_sequence(const uint32_t new_value) {
	data->playdead_sequence = new_value;
}

uint32_t ui_config() {
	uint8_t bloff_value;
	
	// Cap value
	bloff_value = (data->ui_config >> 5) & 7;
	if (bloff_value > 4) bloff_value = 1;		// 15s default

	data->ui_config = (data->ui_config & 0x1F) | (bloff_value << 5);
	
	return data->ui_config;
}

uint16_t ui_config_bloff() {
	uint8_t bloff_value;
	uint16_t bloff_seconds[5] = { 5, 15, 60, 300, 600 };
	
	if (!(data->ui_config & 2)) return 0;

	// Cap value
	bloff_value = (data->ui_config >> 5) & 7;
	if (bloff_value > 4) bloff_value = 1;

	return bloff_seconds[bloff_value];
}

void set_config_textentry(uint8_t new_value) {
	data->ui_config = (data->ui_config & ~0b1100) | ((new_value & 1) << 2);
}

uint8_t ui_config_textentry() {
	return ((data->ui_config >> 2) & 1);
}

void set_ui_config(const uint32_t new_value) {
	data->ui_config = new_value;
}

} /* namespace persistent_memory */
} /* namespace portapack */
