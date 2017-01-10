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

using namespace ui;

struct freqman_entry {
	rf::Frequency value;
	std::string frequency_str;
	std::string description;
};

bool load_freqman_file(std::vector<freqman_entry> &frequencies);
bool save_freqman_file(std::vector<freqman_entry> &frequencies);
bool create_freqman_file(File &freqs_file);
std::string freqman_item_string(freqman_entry &frequencies);

#endif/*__FREQMAN_H__*/
