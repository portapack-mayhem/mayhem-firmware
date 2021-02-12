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

using option_t = std::pair<std::string, int32_t>;
using options_t = std::vector<option_t>;

options_t freqman_entry_modulations = {
		{ "AM", 0 },
		{ "NFM", 1 },
		{ "WFM", 2 }
};

options_t freqman_entry_bandwidths[ 3 ] = {
	{ //AM
		{ "DSB", 0 },
		{ "USB", 1 },
		{ "LSB", 2 },
		{ "CW" , 3 }
	},
	{ //NFM
		{ "8k5" , 0 },
		{ "11k" , 1 },
		{ "16k" , 2 }
	},
	{ //WFM
		{ "16k" , 0 }
	}
};

options_t freqman_entry_steps = {
	{ "5Khz (SA AM)" , 5000 },
	{ "6.25Khz(NFM)" , 6250 },
	{ "8.33Khz(AIR)" , 8330 },
	{ "9Khz (EU AM)" , 9000 },
	{ "10Khz(US AM)" , 10000 },
	{ "12.5Khz(NFM)" , 12500 },
	{ "15Khz  (HFM)" , 15000 },
	{ "25Khz   (N1)" , 25000 },
	{ "50Khz  (FM1)" , 50000 },
	{ "100Khz (FM2)" , 100000 },
	{ "250khz  (N2)" , 250000 },
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
	int8_t modulation = 0 ;
	int8_t bandwidth = 0 ;
	int8_t step = 0 ;
	int8_t tone = 0 ;


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

			modulation = -1 ;
			bandwidth = -1 ;
			step = -1 ;
			tone = -1 ;
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
				modulation = strtoll(pos, nullptr, 10);
			} 
			// bandwidth if any
			pos = strstr(line_start, "b=");
			if (pos) {
				pos += 2;
				bandwidth = strtoll(pos, nullptr, 10);
			} 
			// step if any
			pos = strstr(line_start, "s=");
			if (pos) {
				pos += 2;
				step = strtoll(pos, nullptr, 10);
			} 
			// ctcss tone if any
			pos = strstr(line_start, "c=");
			if (pos) {
				pos += 2;
				tone = strtoll(pos, nullptr, 10);
			} 
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
		if( entry.modulation >= 0 )
		{
			item_string += ",m=" + to_string_dec_uint(entry.modulation);
		}
		if( entry.bandwidth >= 0 )
		{
			item_string += ",b=" + to_string_dec_uint(entry.bandwidth);
		}
		if( entry.step > 0 )
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
			item_string = "HAMRadio: " + entry.description;
			break;
		default:
			item_string = "!UNKNOW TYPE " + entry.description;
			break;
	}

	if (item_string.size() > max_length)
		return item_string.substr(0, max_length - 3) + "...";

	return item_string;
}

void freqman_set_modulation_option( OptionsField &option )
{
	option.set_options( freqman_entry_modulations );
}

void freqman_set_bandwidth_option( int8_t modulation , OptionsField &option )
{
	option.set_options( freqman_entry_bandwidths[ modulation ] );
}

void freqman_set_step_option( OptionsField &option )
{
	option.set_options( freqman_entry_steps );
}

std::string get_freqman_entry_modulation_string( int8_t modulation )
{
	if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
	{
		return std::string( "?M?" ); // unknown modulation
	}
	return freqman_entry_modulations[ modulation ] . first ;
}

std::string get_freqman_entry_bandwidth_string( int8_t modulation , int8_t bandwidth )
{
	if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
	{
		return std::string( "?M?" ); // unknown modulation
	}
	if( bandwidth < 0 || (unsigned)bandwidth > freqman_entry_bandwidths[ modulation ] . size() )
	{
		return std::string( "?B?" ); // unknown modulation
	}
	return freqman_entry_bandwidths[ modulation ][ bandwidth ] . first ;
}

std::string get_freqman_entry_step_string( int8_t step )
{
	if( step < 0 || (unsigned)step >= freqman_entry_steps . size() )
	{
		return std::string( "?S?" ); // unknown modulation
	}
	return freqman_entry_steps[ step ] . first ;
}

int32_t get_freqman_entry_modulation_value( int8_t modulation )
{
	if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
	{
		return -1 ; // unknown modulation
	}
	return freqman_entry_modulations[ modulation ] . second ;
}

int32_t get_freqman_entry_bandwidth_value( int8_t modulation , int8_t bandwidth )
{
	if( modulation < 0 || (unsigned)modulation >= freqman_entry_modulations . size() )
	{
		return -1 ; // unknown modulation
	}
	if( bandwidth < 0 || (unsigned)bandwidth > freqman_entry_bandwidths[ modulation ] . size() )
	{
		return -2 ; // unknown modulation
	}
	return freqman_entry_bandwidths[ modulation ][ bandwidth ] . second ;
}

int32_t get_freqman_entry_step_value( int8_t step )
{
	if( step < 0 || (unsigned)step >= freqman_entry_steps . size() )
	{
		return -1 ; // unknown modulation
	}
	return freqman_entry_steps[ step ] . second ;
}
