/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

constexpr rf::Frequency tuned_frequency_reset_value { 100000000 };

using ppb_range_t = range_t<ppb_t>;
constexpr ppb_range_t ppb_range { -99000, 99000 };
constexpr ppb_t ppb_reset_value { 0 };

using tone_mix_range_t = range_t<int32_t>;
constexpr tone_mix_range_t tone_mix_range { 10, 99 };
constexpr int32_t tone_mix_reset_value { 20 };

using afsk_freq_range_t = range_t<int32_t>;
constexpr afsk_freq_range_t afsk_freq_range { 1, 4000 };
constexpr int32_t afsk_mark_reset_value { 1200 };
constexpr int32_t afsk_space_reset_value { 2200 };

using modem_baudrate_range_t = range_t<int32_t>;
constexpr modem_baudrate_range_t modem_baudrate_range { 50, 9600 };
constexpr int32_t modem_baudrate_reset_value { 1200 };

/*using modem_bw_range_t = range_t<int32_t>;
constexpr modem_bw_range_t modem_bw_range { 1000, 50000 };
constexpr int32_t modem_bw_reset_value { 15000 };*/

using modem_repeat_range_t = range_t<int32_t>;
constexpr modem_repeat_range_t modem_repeat_range { 1, 99 };
constexpr int32_t modem_repeat_reset_value { 5 };

/* struct must pack the same way on M4 and M0 cores. */
struct data_t {
	int64_t tuned_frequency;
	int32_t correction_ppb;
	uint32_t touch_calibration_magic;
	touch::Calibration touch_calibration;

	// Modem
	uint32_t modem_def_index;
	serial_format_t serial_format;
	int32_t modem_bw;
	int32_t afsk_mark_freq;
	int32_t afsk_space_freq;
	int32_t modem_baudrate;
	int32_t modem_repeat;
	
	// Play dead unlock
	uint32_t playdead_magic;
	uint32_t playing_dead;
	uint32_t playdead_sequence;
	
	// UI
	uint32_t ui_config;
	
	uint32_t pocsag_last_address;
	uint32_t pocsag_ignore_address;
	
	int32_t tone_mix;
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

static constexpr uint32_t touch_calibration_magic = 0x074af82f;

void set_touch_calibration(const touch::Calibration& new_value) {
	data->touch_calibration = new_value;
	data->touch_calibration_magic = touch_calibration_magic;
}

const touch::Calibration& touch_calibration() {
	if( data->touch_calibration_magic != touch_calibration_magic ) {
		set_touch_calibration(touch::default_calibration());
	}
	return data->touch_calibration;
}

int32_t tone_mix() {
	tone_mix_range.reset_if_outside(data->tone_mix, tone_mix_reset_value);
	return data->tone_mix;
}

void set_tone_mix(const int32_t new_value) {
	data->tone_mix = tone_mix_range.clip(new_value);
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

int32_t modem_baudrate() {
	modem_baudrate_range.reset_if_outside(data->modem_baudrate, modem_baudrate_reset_value);
	return data->modem_baudrate;
}

void set_modem_baudrate(const int32_t new_value) {
	data->modem_baudrate = modem_baudrate_range.clip(new_value);
}

/*int32_t modem_bw() {
	modem_bw_range.reset_if_outside(data->modem_bw, modem_bw_reset_value);
	return data->modem_bw;
}

void set_modem_bw(const int32_t new_value) {
	data->modem_bw = modem_bw_range.clip(new_value);
}*/

uint8_t modem_repeat() {
	modem_repeat_range.reset_if_outside(data->modem_repeat, modem_repeat_reset_value);
	return data->modem_repeat;
}

void set_modem_repeat(const uint32_t new_value) {
	data->modem_repeat = modem_repeat_range.clip(new_value);
}

serial_format_t serial_format() {
	return data->serial_format;
}

void set_serial_format(const serial_format_t new_value) {
	data->serial_format = new_value;
}

static constexpr uint32_t playdead_magic = 0x88d3bb57;

uint32_t playing_dead() {
	return data->playing_dead;
}

void set_playing_dead(const uint32_t new_value) {
	if( data->playdead_magic != playdead_magic ) {
		set_playdead_sequence(0x8D1);	// U D L R
	}
	data->playing_dead = new_value;
}

uint32_t playdead_sequence() {
	if( data->playdead_magic != playdead_magic ) {
		set_playdead_sequence(0x8D1);	// U D L R
	}
	return data->playdead_sequence;
}

void set_playdead_sequence(const uint32_t new_value) {
	data->playdead_sequence = new_value;
	data->playdead_magic = playdead_magic;
}

bool config_speaker() {
	return (data->ui_config & 0x8000000UL) ? true : false;
}

bool config_backbutton() {
	return (data->ui_config & 0x10000000UL) ? true : false;
}

bool stealth_mode() {
	return (data->ui_config & 0x20000000UL) ? true : false;
}

bool config_login() {
	return (data->ui_config & 0x40000000UL) ? true : false;
}

bool config_splash() {
	return (data->ui_config & 0x80000000UL) ? true : false;
}

uint32_t config_backlight_timer() {
	const uint32_t timer_seconds[8] = { 0, 5, 15, 60, 300, 600, 600, 600 };

	return timer_seconds[data->ui_config & 0x00000007UL];
}

void set_config_speaker(bool v) {
	data->ui_config = (data->ui_config & ~0x8000000UL) | (v << 27); 
}

void set_config_backbutton(bool v) {
	data->ui_config = (data->ui_config & ~0x10000000UL) | (v << 28); 
}

void set_stealth_mode(const bool v) {
	data->ui_config = (data->ui_config & ~0x20000000UL) | (v << 29);
}

void set_config_login(bool v) {
	data->ui_config = (data->ui_config & ~0x40000000UL) | (v << 30);
}

void set_config_splash(bool v) {
	data->ui_config = (data->ui_config & ~0x80000000UL) | (v << 31);
}

void set_config_backlight_timer(uint32_t i) {
	data->ui_config = (data->ui_config & ~0x00000007UL) | (i & 7);
}

/*void set_config_textentry(uint8_t new_value) {
	data->ui_config = (data->ui_config & ~0b100) | ((new_value & 1) << 2);
}

uint8_t ui_config_textentry() {
	return ((data->ui_config >> 2) & 1);
}*/

/*void set_ui_config(const uint32_t new_value) {
	data->ui_config = new_value;
}*/

uint32_t pocsag_last_address() {
	return data->pocsag_last_address;
}

void set_pocsag_last_address(uint32_t address) {
	data->pocsag_last_address = address;
}

uint32_t pocsag_ignore_address() {
	return data->pocsag_ignore_address;
}

void set_pocsag_ignore_address(uint32_t address) {
	data->pocsag_ignore_address = address;
}

} /* namespace persistent_memory */
} /* namespace portapack */
