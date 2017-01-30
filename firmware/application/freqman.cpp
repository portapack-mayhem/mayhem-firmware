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

bool load_freqman_file(std::vector<freqman_entry> &frequencies) {
	File freqs_file;
	size_t end, n = 0;
	char * file_buffer;
	char * desc_pos;
	char desc_buffer[32] = { 0 };
	std::string description;
	rf::Frequency value;
	
	file_buffer = (char *)chHeapAlloc(0, 2048);
	
	while (freqs_file.open("freqman.txt").is_valid()) {
		auto result = freqs_file.create("freqman.txt");
		if (result.is_valid())
			return false;
	}
	
	freqs_file.read(file_buffer, 2048);

	char * pos = file_buffer;
	while ((pos = strstr(pos, "f=")) && n < 32) {
		pos += 2;
		
		value = strtol(pos, nullptr, 10);

		desc_pos = strstr(pos, "d=");
		if (desc_pos) {
			desc_pos += 2;
			end = strcspn(desc_pos, ",\x0D\x0A");		// CR LF
			if (end > 31) end = 31;
			memcpy(desc_buffer, desc_pos, end);
			desc_buffer[end] = (char)0;
			description = desc_buffer;
			pos = desc_pos;
		} else {
			description = "-";
		}
		
		frequencies.push_back({ value, "", description });
	}
	
	chHeapFree(file_buffer);
	
	return true;
}

bool save_freqman_file(std::vector<freqman_entry> &frequencies) {
	File freqs_file;
	size_t n;
	std::string item_string;
	
	if (!create_freqman_file(freqs_file)) return false;
	
	for (n = 0; n < frequencies.size(); n++) {
		item_string = "f=" + to_string_dec_uint(frequencies[n].value);
		
		if (frequencies[n].description.size())
			item_string += ",d=" + frequencies[n].description;
		
		freqs_file.write_line(item_string);
	}
	
	return true;
}

bool create_freqman_file(File &freqs_file) {
	auto result = freqs_file.create("freqman.txt");
	if (result.is_valid()) return false;
	
	return true;
}

std::string freqman_item_string(freqman_entry &entry) {
	std::string item_string, frequency_str, description;
	char temp_buffer[32];
	rf::Frequency value;
	
	value = entry.value;
	entry.frequency_str = to_string_dec_int(value / 1000000, 4) + "." +
							to_string_dec_int((value / 100) % 10000, 4, '0');
	
	if (entry.description.size() <= 19) {
		item_string = entry.frequency_str + ":" + entry.description;
	} else {
		memcpy(temp_buffer, entry.description.c_str(), 16);
		temp_buffer[16] = (char)0;
		item_string = entry.frequency_str + ":" + temp_buffer + "...";
	}
	
	return item_string;
}
