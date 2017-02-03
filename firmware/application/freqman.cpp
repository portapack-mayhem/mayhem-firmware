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

#define FREQMAN_MAX_PER_CAT 64
#define FREQMAN_CAT_MAX_LEN 8
#define FREQMAN_DESC_MAX_LEN 32

bool load_freqman_file(freqman_db &db) {
	File freqs_file;
	size_t length, span_end, n = 0;
	uint64_t seek_pos = 0;
	char * line_end;
	int32_t category_id;
	char * pos;
	char desc_buffer[FREQMAN_DESC_MAX_LEN + 1] = { 0 };
	char category_buffer[FREQMAN_CAT_MAX_LEN + 1] = { 0 };
	std::string description;
	rf::Frequency value;
	std::vector<std::string>::iterator category_find;
	
	char file_buffer[256];
	
	while (freqs_file.open("freqman.txt").is_valid()) {
		auto result = freqs_file.create("freqman.txt");
		if (result.is_valid())
			return false;
	}
	
	freqs_file.read(file_buffer, 256);
	
	while ((pos = strstr(file_buffer, "f=")) && (n < FREQMAN_MAX_PER_CAT)) {
		
		// Cut buffer at end of line
		line_end = file_buffer;
		do {
			span_end = strcspn(line_end, "\x0D\x0A");
			line_end += (span_end + 1);
		} while (span_end);
		*line_end = (char)0;
		
		pos += 2;
		value = strtol(pos, nullptr, 10);				// Read frequency

		pos = strstr(file_buffer, "d=");
		if (pos) {
			pos += 2;
			length = strcspn(pos, ",\x0D\x0A");			// Read description until , or CR LF
			if (length > FREQMAN_DESC_MAX_LEN) length = FREQMAN_DESC_MAX_LEN;
			memcpy(desc_buffer, pos, length);
			desc_buffer[length] = (char)0;
			description = desc_buffer;
		} else {
			description = "-";
		}
		
		pos = strstr(file_buffer, "c=");
		if (pos) {
			pos += 2;
			length = strcspn(pos, ",\x0D\x0A");			// Read category name until , or CR LF
			if (length > FREQMAN_CAT_MAX_LEN) length = FREQMAN_CAT_MAX_LEN;
			memcpy(category_buffer, pos, length);
			category_buffer[length] = (char)0;
			
			// See if we already know that category
			category_find = find(db.categories.begin(), db.categories.end(), category_buffer);
			if (category_find == db.categories.end()) {
				// Not found: add to list
				db.categories.push_back(category_buffer);
				category_id = db.categories.size() - 1;
			} else {
				// Found
				category_id = category_find - db.categories.begin();
			}
		} else {
			category_id = -1;							// Uncategorized
		}
		
		db.entries.push_back({ value, "", description, category_id });
		n++;

		seek_pos += (line_end - file_buffer);

		if (freqs_file.seek(seek_pos).value() == seek_pos)
			break;
		else
			freqs_file.read(file_buffer, 256);
	}
	
	//chHeapFree(file_buffer);
	
	return true;
}

bool save_freqman_file(freqman_db &db) {
	File freqs_file;
	size_t n;
	std::string item_string;
	int32_t category_id;
	
	if (!create_freqman_file(freqs_file)) return false;
	
	for (n = 0; n < db.entries.size(); n++) {
		item_string = "f=" + to_string_dec_uint(db.entries[n].value);
		
		if (db.entries[n].description.size())
			item_string += ",d=" + db.entries[n].description;
		
		category_id = db.entries[n].category_id;
		if ((category_id >= 0) && (category_id < (int32_t)db.categories.size()))
			item_string += ",c=" + db.categories[db.entries[n].category_id];
		
		freqs_file.write_line(item_string);
	}
	
	return true;
}

bool create_freqman_file(File &freqs_file) {
	auto result = freqs_file.create("freqman.txt");
	if (result.is_valid())
		return false;
	
	return true;
}

std::string freqman_item_string(freqman_entry &entry) {
	std::string item_string, frequency_str, description;
	rf::Frequency value;
	
	value = entry.value;
	entry.frequency_str = to_string_dec_int(value / 1000000, 4) + "." +
							to_string_dec_int((value / 100) % 10000, 4, '0');
	
	item_string = entry.frequency_str + "M: ";
	
	if (entry.description.size() <= 19) {
		item_string += entry.description;
	} else {
		// Cut if too long
		item_string += entry.description.substr(0, 16) + "...";
	}
	
	return item_string;
}
