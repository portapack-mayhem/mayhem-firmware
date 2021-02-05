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

#ifndef __FREQMAN_H__
#define __FREQMAN_H__

#include <cstring>
#include <string>
#include "file.hpp"
#include "ui_receiver.hpp"
#include "string_format.hpp"
#include "ui_widget.hpp"
#include "enum_factory.hpp"

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
	SINGLE = 0,	//f=
	RANGE,		//a=,b=
	HAMRADIO,	//r=,t=
	ERROR_TYPE
};


/*
 //add new entries before DEFAULT
#define FREQMAN_ENTRY_MODULATION(ENTRY) \
    ENTRY(MOD_AM,=0) \
    ENTRY(MOD_NFM,) \
    ENTRY(MOD_WFM,) \
    ENTRY(DEFAULT_MODULATION,=-1) \
    ENTRY(ERROR_MODULATION,=-2)
DECLARE_ENUM( freqman_entry_modulation , FREQMAN_ENTRY_MODULATION );

 //add new entries before DEFAULT
#define FREQMAN_ENTRY_BANDWIDTH_AM(ENTRY) \
    ENTRY(AM_DSB,) \
    ENTRY(AM_USB,) \
    ENTRY(AM_LSB,) \
    ENTRY(AM_CW,) \
    ENTRY(DEFAULT_BANDWIDTH_AM,=-1) \
    ENTRY(ERROR_BANDWIDTH_AM,=-2)
DECLARE_ENUM( freqman_entry_bandwidth_am , FREQMAN_ENTRY_BANDWIDTH_AM );

 //add new entries before DEFAULT
#define FREQMAN_ENTRY_BANDWIDTH_NFM(ENTRY) \
    ENTRY(NFM_8k5,) \
    ENTRY(NFM_11k,) \
    ENTRY(NFM_16k,) \
    ENTRY(DEFAULT_BANDWIDTH_NFM,=-1) \
    ENTRY(ERROR_BANDWIDTH_NFM,=-2)
DECLARE_ENUM( freqman_entry_bandwidth_nfm , FREQMAN_ENTRY_BANDWIDTH_NFM );

//add new entries before DEFAULT
#define FREQMAN_ENTRY_BANDWIDTH_WFM(ENTRY) \
    ENTRY(WFM_16k,) \
    ENTRY(DEFAULT_BANDWIDTH_WFM,=-1) \
    ENTRY(ERROR_BANDWIDTH_WFM,=-2)
DECLARE_ENUM( freqman_entry_bandwidth_wfm , FREQMAN_ENTRY_BANDWIDTH_WFM );

 //add new entries before DEFAULT
#define FREQMAN_ENTRY_STEP(ENTRY) \
	ENTRY(SA_AM,=5000) \
	ENTRY(NFM_2,=6250) \
	ENTRY(AIRBAND,=8330) \
	ENTRY(AM_EUR,=9000) \
	ENTRY(AM_US,=10000) \
	ENTRY(NFM_1,=12500) \
	ENTRY(N_1,=25000) \
	ENTRY(FM_2,=50000) \
	ENTRY(FM_1,=100000) \
	ENTRY(N_2,=250000) \
	ENTRY(DEFAULT_STEP,=-1) \
	ENTRY(ERROR_STEP,=-2)
DECLARE_ENUM( freqman_entry_step , FREQMAN_ENTRY_STEP );
*/

struct freqman_entry {
	rf::Frequency frequency_a { 0 };	// 'f=freq' or 'a=freq_start' or 'r=recv_freq'
	rf::Frequency frequency_b { 0 };	// 'b=freq_end' or 't=tx_freq'
	std::string description { };		// 'd=desc'
	freqman_entry_type type { };		// SINGLE,RANGE,HAMRADIO
	int8_t modulation { };	// AM,NFM,WFM
	int8_t bandwidth { };	// AM_DSB, ...
	int8_t step { };
	uint16_t tone { };
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
