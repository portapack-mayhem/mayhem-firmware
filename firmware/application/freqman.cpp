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

#define FREQMAN_DESC_MAX_LEN 30
#define FREQMAN_MAX_PER_FILE 256

std::vector<std::string> get_freqman_files() {
	std::vector<std::string> file_list;
	
	auto files = scan_root_files(u"FREQMAN", u"*.TXT");
	
	for (auto file : files) {
		file_list.emplace_back(file.stem().string());
	}
	
	return file_list;
};

bool load_freqman_file(std::string& file_stem, freqman_db &db) {
	File freqman_file;
	size_t length, span_end, n = 0;
	uint64_t seek_pos = 0;
	char * pos;
	char * line_end;
	std::string description;
	rf::Frequency value;
	char file_data[256];
	
	db.entries.clear();
	
	auto result = freqman_file.open("FREQMAN/" + file_stem + ".TXT");
	if (result.is_valid())
		return false;
	
	freqman_file.read(file_data, 256);
	
	while ((pos = strstr(file_data, "f=")) && (n < FREQMAN_MAX_PER_FILE)) {
		
		// Trim buffer at end of last complete line
		line_end = file_data;
		span_end = strcspn(line_end, "\x0A");
		if (span_end) {
			if (span_end == strlen(line_end))
				return true;
		}
		line_end += (span_end + 1);
		*line_end = (char)0;
		
		// Read frequency
		pos += 2;
		value = strtoll(pos, nullptr, 10);

		// Read description until , or LF
		pos = strstr(file_data, "d=");
		if (pos) {
			pos += 2;
			length = std::min(strcspn(pos, ",\x0A"), (size_t)FREQMAN_DESC_MAX_LEN);
			description = string(pos, length);
		} else {
			description = "-";
		}
		
		db.entries.push_back({ value, "", description });
		n++;

		seek_pos += (line_end - file_data);

		if (freqman_file.seek(seek_pos).value() == seek_pos)
			break;
		else
			freqman_file.read(file_data, 256);
	}
	
	return true;
}

bool save_freqman_file(std::string& file_stem, freqman_db &db) {
	File freqman_file;
	std::string item_string;
	rf::Frequency f;
	
	if (!create_freqman_file(file_stem, freqman_file))
		return false;
	
	for (size_t n = 0; n < db.entries.size(); n++) {
		f = db.entries[n].value;
		item_string = "f=" + to_string_dec_uint(f / 1000) + to_string_dec_uint(f % 1000UL, 3, '0');		// Please forgive me
		
		if (db.entries[n].description.size())
			item_string += ",d=" + db.entries[n].description;
		
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
	std::string item_string, frequency_str, description;
	rf::Frequency value;
	
	value = entry.value;
	entry.frequency_str = to_string_dec_int(value / 1000000, 4) + "." +
							to_string_dec_int((value / 100) % 10000, 4, '0');
	
	item_string = entry.frequency_str + "M: " + entry.description;
	
	if (entry.description.size() > max_length)
		return item_string.substr(0, max_length - 3) + "...";
	
	return item_string;
}
