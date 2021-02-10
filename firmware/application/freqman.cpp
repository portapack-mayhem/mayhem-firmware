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
			
			db.push_back({ frequency_a, frequency_b, description, type });
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
			
		} else {
			// Range
			frequency_b = entry.frequency_b;
			
			item_string = "a=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
			item_string += ",b=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
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

	if (entry.type == SINGLE) {
		item_string = to_string_short_freq(entry.frequency_a) + "M: " + entry.description;
	} else {
		item_string = "Range: " + entry.description;
	}
	
	if (item_string.size() > max_length)
		return item_string.substr(0, max_length - 3) + "...";
	
	return item_string;
}
