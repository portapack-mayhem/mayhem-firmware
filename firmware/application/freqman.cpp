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

#include "freqman.hpp"
#include <algorithm>

// Bandwidth sorted by frequman_entry_modulation value 
std::vector<std::vector<std::string>> freqman_entry_bandwidth {
		{"DSB","USB","LSB","CW"},
		{"8k5","11k","16k"},
		{"16k"},
		{"DEF"},
		{"ERR"}
};

// Step names
std::vector<string> freqman_entry_step_names {
		"AM_US"   ,			
		"AM_EUR"  ,
		"NFM_1"   ,
		"NFM_2",
		"FM_1",
		"FM_2",
		"N_1",
		"N_2",
		"AIRBAND",
		"STEP_DEF",
		"ERROR_STEP"
};

std::vector<std::string> get_freqman_files() {
	std::vector<std::string> file_list;

	auto files = scan_root_files(u"FREQMAN", u"*.TXT");

	for (auto file : files) {
		std::string file_name = file.stem().string();
		// don't propose tmp / hidden files in freqman's list
		if (file_name.length() && file_name[0] != '.') {
			file_list.emplace_back(file_name);
		}
	}

	return file_list;
};

bool load_freqman_file(std::string& file_stem, freqman_db &db) {
	File freqman_file;
	size_t length, n = 0, file_position = 0;
	char * pos;
	char * line_start;
	char * line_end;
	std::string description;
	rf::Frequency frequency_a, frequency_b;
	char file_data[257];
	freqman_entry_type type;

	db.clear();

	auto result = freqman_file.open("FREQMAN/" + file_stem + ".TXT");
	if (result.is_valid())
		return false;

	while (1) {
		// Read a 256 bytes block from file
		freqman_file.seek(file_position);

		memset(file_data, 0, 257);
		auto read_size = freqman_file.read(file_data, 256);
		if (read_size.is_error())
			return false;	// Read error

		file_position += 256;

		// Reset line_start to beginning of buffer
		line_start = file_data;

		if (!strstr(file_data, "f=") && !strstr(file_data, "a="))
			break;

		// Look for complete lines in buffer
		while ((line_end = strstr(line_start, "\x0A"))) {
			// Read frequency
			pos = strstr(line_start, "f=");
			frequency_b = 0;
			type = SINGLE;
			if (pos) {
				pos += 2;
				frequency_a = strtoll(pos, nullptr, 10);
			} else {
				// ...or range
				pos = strstr(line_start, "a=");
				if (pos) {
					pos += 2;
					frequency_a = strtoll(pos, nullptr, 10);
					type = RANGE;
					pos = strstr(line_start, "b=");
					if (pos) {
						pos += 2;
						frequency_b = strtoll(pos, nullptr, 10);
					} else
						frequency_b = 0;
				} else
					frequency_a = 0;
			}

			// Read description until , or LF
			pos = strstr(line_start, "d=");
			if (pos) {
				pos += 2;
				length = std::min(strcspn(pos, ",\x0A"), (size_t)FREQMAN_DESC_MAX_LEN);
				description = string(pos, length);
			} else
				description = "-";

			db.push_back({ frequency_a, frequency_b, description, type , 0 , 0 , 0 , 0 });
			n++;

			if (n >= FREQMAN_MAX_PER_FILE) return true;

			line_start = line_end + 1;
			if (line_start - file_data >= 256) break;
		}

		if (read_size.value() != 256)
			break;	// End of file

		// Restart at beginning of last incomplete line
		file_position -= (file_data + 256 - line_start);
	}

	return true;
}

bool load_freqman_file_ex(std::string& file_stem, freqman_db& db, bool load_freqs , bool load_ranges , bool load_hamradios ) {
	File freqman_file;
	size_t length, n = 0, file_position = 0;
	char * pos;
	char * line_start;
	char * line_end;
	std::string description;
	rf::Frequency frequency_a, frequency_b;
	char file_data[257];
	freqman_entry_type type;
	uint8_t modulation = 0 ;
	uint8_t bandwidth = 0 ;
	uint32_t step = 0 ;
	uint16_t tone = 0 ;


	db.clear();

	auto result = freqman_file.open("FREQMAN/" + file_stem + ".TXT");
	if (result.is_valid())
		return false;

	while (1) {
		// Read a 256 bytes block from file
		freqman_file.seek(file_position);

		memset(file_data, 0, 257);
		auto read_size = freqman_file.read(file_data, 256);
		if (read_size.is_error())
			return false;	// Read error

		file_position += 256;

		// Reset line_start to beginning of buffer
		line_start = file_data;

		if (!strstr(file_data, "f=") && !strstr(file_data, "a=") && !strstr(file_data, "r=") )
			break;

		// Look for complete lines in buffer
		while ((line_end = strstr(line_start, "\x0A"))) {

			modulation = 0 ;
			bandwidth = 0 ;
			step = 0 ;
			tone = 0 ;
			type=SINGLE ;

			frequency_a = frequency_b = 0;
			// Read frequency
			pos = strstr(line_start, "f=");
			if(pos) {
				pos += 2;
				frequency_a = strtoll(pos, nullptr, 10);
			} else {
				// ...or range
				pos = strstr(line_start, "a=");
				if (pos) {
					pos += 2;
					frequency_a = strtoll(pos, nullptr, 10);
					type = RANGE;
					pos = strstr(line_start, "b=");
					if (pos) {
						pos += 2;
						frequency_b = strtoll(pos, nullptr, 10);
					} else
						frequency_b = 0;
				}else {
					// ... or hamradio
					pos = strstr(line_start, "r=");
					if (pos) {
						pos += 2;
						frequency_a = strtoll(pos, nullptr, 10);
						type = HAMRADIO;
						pos = strstr(line_start, "t=");
						if (pos) {
							pos += 2;
							frequency_b = strtoll(pos, nullptr, 10);
						} else
							frequency_b = frequency_a ;
					} else
						frequency_a = 0;
				}
			}
			// modulation if any
			pos = strstr(line_start, "m=");
			if (pos) {
				pos += 2;
				modulation = static_cast<short>(strtoll(pos, nullptr, 10));
			} else
				modulation = 0;
			// bandwidth if any
			pos = strstr(line_start, "b=");
			if (pos) {
				pos += 2;
				bandwidth = static_cast<short>(strtoll(pos, nullptr, 10));
			} else
				bandwidth = 0;
			// step if any
			pos = strstr(line_start, "s=");
			if (pos) {
				pos += 2;
				step = strtoll(pos, nullptr, 10);
			} else
				step = 0;
			// ctcss tone if any
			pos = strstr(line_start, "c=");
			if (pos) {
				pos += 2;
				tone = strtoll(pos, nullptr, 10);
			} else
				tone = 0;

			// Read description until , or LF
			pos = strstr(line_start, "d=");
			if (pos) {
				pos += 2;
				length = std::min(strcspn(pos, ",\x0A"), (size_t)FREQMAN_DESC_MAX_LEN);
				description = string(pos, length);
			} else
				description = "-";
			if( (type == SINGLE && load_freqs) || (type == RANGE && load_ranges) || (type == HAMRADIO && load_hamradios) )
			{
				db.push_back({ frequency_a, frequency_b, description, type , modulation , bandwidth , step , tone });
				n++;
				if (n >= FREQMAN_MAX_PER_FILE) return true;
			}

			line_start = line_end + 1;
			if (line_start - file_data >= 256) break;
		}

		if (read_size.value() != 256)
			break;	// End of file

		// Restart at beginning of last incomplete line
		file_position -= (file_data + 256 - line_start);
	}

	return true;
}


bool save_freqman_file(std::string& file_stem, freqman_db &db) {
	File freqman_file;
	std::string item_string;
	rf::Frequency frequency_a, frequency_b;

	if (!create_freqman_file(file_stem, freqman_file))
		return false;

	for (size_t n = 0; n < db.size(); n++) {
		auto& entry = db[n];

		frequency_a = entry.frequency_a;

		if (entry.type == SINGLE) {
			// Single

			// TODO: Make to_string_dec_uint be able to return uint64_t's
			// Please forgive me...
			item_string = "f=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');

		} else if( entry.type == RANGE ) {
			// Range
			frequency_b = entry.frequency_b;
			item_string = "a=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
			item_string += ",b=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
		} else if( entry.type == HAMRADIO ) {
			frequency_b = entry.frequency_b;
			item_string = "r=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
			item_string += ",t=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
		}
		if( entry.modulation != 0 )
		{
			item_string += ",m=" + to_string_dec_uint(entry.modulation);
		}
		if( entry.bandwidth != 0 )
		{
			item_string += ",b=" + to_string_dec_uint(entry.bandwidth);
		}
		if( entry.step != 0 )
		{
			item_string += ",s=" + to_string_dec_uint(entry.step);
		}
		if( entry.tone != 0 )
		{
			item_string += ",c=" + to_string_dec_uint(entry.tone);
		}

		if (entry.description.size())
			item_string += ",d=" + entry.description;

		freqman_file.write_line(item_string);
	}

	return true;
}

bool create_freqman_file(std::string& file_stem, File& freqman_file) {
	auto result = freqman_file.create("FREQMAN/" + file_stem + ".TXT");
	if (result.is_valid())
		return false;

	return true;
}

std::string freqman_item_string(freqman_entry &entry, size_t max_length) {
	std::string item_string;

	switch( entry.type ){
		case SINGLE:
			item_string = to_string_short_freq(entry.frequency_a) + "M: " + entry.description;
		break;
		case RANGE:
			item_string = "Range: " + entry.description;
		break;
		case HAMRADIO:
			item_string = "HAMradio: " + entry.description;
		break;
		default:
			item_string = "!UNKNOW TYPE " + entry.description;
		break;
	}

	if (item_string.size() > max_length)
		return item_string.substr(0, max_length - 3) + "...";

	return item_string;
}

bool freqman_set_bandwidth_option( int8_t modulation , OptionsField &option )
{
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t bw ;
	switch( modulation )
	{
		case AM:
			bw.emplace_back("DSB", 0);
			bw.emplace_back("USB", 0);
			bw.emplace_back("LSB", 0);
			bw.emplace_back("CW ", 0);
			option.set_options(bw);
			break;
		case WFM:
			bw.emplace_back("16k", 0);
			option.set_options(bw);
			break;
		case NFM:
			bw.emplace_back("8k5", 0);
			bw.emplace_back("11k", 0);
			bw.emplace_back("16k", 0);			
			option.set_options(bw);
			break;

		default:
			return false ;
			break;
	}	
	return true ;
}

bool freqman_set_modulation_option( OptionsField &option )
{
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t bw ;
	bw.emplace_back("AM", 0);
	bw.emplace_back("WFM", 1);
	bw.emplace_back("NFM", 2);			
	option.set_options(bw);
	return true ;
}

bool freqman_set_step_option( OptionsField &option )
{
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t bw ;
	bw.emplace_back( "5Khz (SA AM)",	5000 );
	bw.emplace_back( "9Khz (EU AM)",	9000 );
	bw.emplace_back( "10Khz(US AM)",	10000 );
	bw.emplace_back( "50Khz (FM1)", 	50000 );
	bw.emplace_back( "100Khz(FM2)", 	100000 );
	bw.emplace_back( "6.25khz(NFM)",	6250 );
	bw.emplace_back( "12.5khz(NFM)",	12500 );
	bw.emplace_back( "25khz (N1)",		25000 );
	bw.emplace_back( "250khz (N2)",		250000 );
	bw.emplace_back( "8.33khz(AIR)",	8330 );
	bw.emplace_back( "15 khz(HFM)",		15000 );
	option.set_options(bw);
	return true ;
}
