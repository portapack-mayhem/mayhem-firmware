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

#include <cstring>
#include <string>
#include "file.hpp"
#include "ui_receiver.hpp"
#include "string_format.hpp"
#include "ui_widget.hpp"

#ifndef __FREQMAN_H__
#define __FREQMAN_H__

#define FREQMAN_DESC_MAX_LEN 30
#define FREQMAN_MAX_PER_FILE 99
#define FREQMAN_MAX_PER_FILE_STR "99"

using namespace ui;
using namespace std;

enum freqman_error {
	NO_ERROR = 0,
	ERROR_ACCESS,
	ERROR_NOFILES,
	ERROR_DUPLICATE
};

enum freqman_entry_type {
	SINGLE = 0,
	RANGE,
	HAMRADIO,
	ERROR_TYPE
};

enum freqman_entry_modulation {
	AM,
	NFM,
	WFM,
	MOD_DEF,
	ERROR_MOD
};

//Entry step placed for AlainD freqman version (or any other enhanced version)
enum freqman_entry_step {
	AM_US,			// 10 Khz   AM/CB
	AM_EUR,			// 9 Khz	LW/MW
	NFM_1,			// 12,5 Khz (Analogic PMR 446)
	NFM_2,			// 6,25 Khz  (Digital PMR 446)
	FM_1,			// 100 Khz
	FM_2,			// 50 Khz
	N_1,			// 25 Khz
	N_2,			// 250 Khz
	AIRBAND,		// AIRBAND 8,33 Khz
	STEP_DEF,		// Use default app step
	ERROR_STEP
};

// freqman_entry_step step added, as above, to provide compatibility / future enhancement.
struct freqman_entry {
	rf::Frequency frequency_a { 0 };
	rf::Frequency frequency_b { 0 };
	std::string description { };
	freqman_entry_type type { };
	int8_t modulation { 0 };
	int8_t bandwidth { 0 };
	uint32_t step { 0 };
	uint16_t tone { 0 };
};

using freqman_db = std::vector<freqman_entry>;

std::vector<std::string> get_freqman_files();
bool load_freqman_file(std::string& file_stem, freqman_db& db);
bool load_freqman_file_ex(std::string& file_stem, freqman_db& db, bool load_freqs , bool load_ranges , bool load_hamradios );
bool save_freqman_file(std::string& file_stem, freqman_db& db);
bool create_freqman_file(std::string& file_stem, File& freqman_file);
std::string freqman_item_string(freqman_entry &item, size_t max_length);
bool freqman_set_bandwidth_option( int8_t modulation , OptionsField &option );
bool freqman_set_modulation_option( OptionsField &option );
bool freqman_set_step_option( OptionsField &option );

#endif/*__FREQMAN_H__*/
