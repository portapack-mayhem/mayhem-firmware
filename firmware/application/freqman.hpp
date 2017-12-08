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

#ifndef __FREQMAN_H__
#define __FREQMAN_H__

#define FREQMAN_DESC_MAX_LEN 30
#define FREQMAN_MAX_PER_FILE 50
#define FREQMAN_MAX_PER_FILE_STR "50"

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
	RANGE
};

struct freqman_entry {
	rf::Frequency frequency_a { 0 };
	rf::Frequency frequency_b { 0 };
	std::string description { };
	freqman_entry_type type { };
};

struct freqman_db {
	std::vector<freqman_entry> entries;
};

std::vector<std::string> get_freqman_files();
bool load_freqman_file(std::string& file_stem, freqman_db &db);
bool save_freqman_file(std::string& file_stem, freqman_db &db);
bool create_freqman_file(std::string& file_stem, File& freqman_file);
std::string freqman_item_string(freqman_entry &item, size_t max_length);

#endif/*__FREQMAN_H__*/
