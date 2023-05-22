/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef __FREQMAN_H__
#define __FREQMAN_H__

#include <cstring>
#include <string>
#include "file.hpp"
#include "ui_receiver.hpp"
#include "tone_key.hpp"
#include "string_format.hpp"
#include "ui_widget.hpp"

#define FREQMAN_DESC_MAX_LEN 24         // This is the number of characters that can be drawn in front of "R: TEXT..." before taking a full screen line
#define FREQMAN_MAX_PER_FILE 115        // Maximum of entries we can read. This is a hardware limit
                                        // It was tested and lowered to leave a bit of space to the caller
#define FREQMAN_MAX_PER_FILE_STR "115"  // STRING OF FREQMAN_MAX_PER_FILE

using namespace ui;
using namespace std;
using namespace tonekey;

// needs to be signed as -1 means not set
typedef int8_t freqman_index_t;

enum freqman_error {
    NO_ERROR = 0,
    ERROR_ACCESS,
    ERROR_NOFILES,
    ERROR_DUPLICATE
};

enum freqman_entry_type {
    SINGLE = 0,  // f=
    RANGE,       // a=,b=
    HAMRADIO,    // r=,t=
    ERROR_TYPE
};

enum freqman_entry_modulation {
    AM_MODULATION = 0,
    NFM_MODULATION,
    WFM_MODULATION,
    MODULATION_DEF,
    ERROR_MODULATION
};

// Entry step placed for AlainD freqman version (or any other enhanced version)
enum freqman_entry_step {
    STEP_DEF = -1,  // default
    AM_US,          // 10 kHz   AM/CB
    AM_EUR,         // 9 kHz	LW/MW
    NFM_1,          // 12,5 kHz (Analogic PMR 446)
    NFM_2,          // 6,25 kHz  (Digital PMR 446)
    FM_1,           // 100 kHz
    FM_2,           // 50 kHz
    N_1,            // 25 kHz
    N_2,            // 250 kHz
    AIRBAND,        // AIRBAND 8,33 kHz
    ERROR_STEP
};

struct freqman_entry {
    rf::Frequency frequency_a{0};  // 'f=freq' or 'a=freq_start' or 'r=recv_freq'
    rf::Frequency frequency_b{0};  // 'b=freq_end' or 't=tx_freq'
    std::string description{};     // 'd=desc'
    freqman_entry_type type{};     // SINGLE,RANGE,HAMRADIO
    freqman_index_t modulation{};  // AM,NFM,WFM
    freqman_index_t bandwidth{};   // AM_DSB, ...
    freqman_index_t step{};        // 5khz (SA AM,...
    tone_index tone{};             // 0XZ, 11 1ZB,...
};

using freqman_db = std::vector<freqman_entry>;

std::vector<std::string> get_freqman_files();
bool load_freqman_file(std::string& file_stem, freqman_db& db);
bool load_freqman_file_ex(std::string& file_stem, freqman_db& db, bool load_freqs, bool load_ranges, bool load_hamradios);
bool get_freq_string(freqman_entry& entry, std::string& item_string);
bool save_freqman_file(std::string& file_stem, freqman_db& db);
bool create_freqman_file(std::string& file_stem, File& freqman_file);

std::string freqman_item_string(freqman_entry& item, size_t max_length);

void freqman_set_bandwidth_option(freqman_index_t modulation, OptionsField& option);
void freqman_set_modulation_option(OptionsField& option);
void freqman_set_step_option(OptionsField& option);
void freqman_set_step_option_short(OptionsField& option);
void freqman_set_tone_option(OptionsField& option);

std::string freqman_entry_get_modulation_string(freqman_index_t modulation);
std::string freqman_entry_get_bandwidth_string(freqman_index_t modulation, freqman_index_t bandwidth);
std::string freqman_entry_get_step_string(freqman_index_t step);
std::string freqman_entry_get_step_string_short(freqman_index_t step);

int32_t freqman_entry_get_modulation_value(freqman_index_t modulation);
int32_t freqman_entry_get_bandwidth_value(freqman_index_t modulation, freqman_index_t bandwidth);
int32_t freqman_entry_get_step_value(freqman_index_t step);

freqman_index_t freqman_entry_get_modulation_from_str(char* str);
freqman_index_t freqman_entry_get_bandwidth_from_str(freqman_index_t modulation, char* str);
freqman_index_t freqman_entry_get_step_from_str(char* str);
freqman_index_t freqman_entry_get_step_from_str_short(char* str);

#endif /*__FREQMAN_H__*/
