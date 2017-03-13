/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ctcss.hpp"
#include "string_format.hpp"

namespace ctcss {
	
const ctcss_tone ctcss_tones[CTCSS_TONES_NB] = {
	{ "XZ", 0, 67.000 },
	{ "WZ", 1, 69.400 },
	{ "XA", 2, 71.900 },
	{ "WA", 3, 74.400 },
	{ "XB", 4, 77.000 },
	{ "WB", 5, 79.700 },
	{ "YZ", 6, 82.500 },
	{ "YA", 7, 85.400 },
	{ "YB", 8, 88.500 },
	{ "ZZ", 9, 91.500 },
	{ "ZA", 10, 94.800 },
	{ "ZB", 11, 97.400 },
	{ "1Z", 12, 100.000 },
	{ "1A", 13, 103.500 },
	{ "1B", 14, 107.200 },
	{ "2Z", 15, 110.900 },
	{ "2Z", 16, 114.800 },
	{ "2B", 17, 118.800 },
	{ "3Z", 18, 123.000 },
	{ "3A", 19, 127.300 },
	{ "3B", 20, 131.800 },
	{ "4Z", 21, 136.500 },
	{ "4A", 22, 141.300 },
	{ "4B", 23, 146.200 },
	{ "5Z", 24, 151.400 },
	{ "5A", 25, 156.700 },
	{ "--", 40, 159.800 },
	{ "5B", 26, 162.200 },
	{ "--", 41, 165.500 },
	{ "6Z", 27, 167.900 },
	{ "--", 42, 171.300 },
	{ "6A", 28, 173.800 },
	{ "--", 43, 177.300 },
	{ "6B", 29, 179.900 },
	{ "--", 44, 183.500 },
	{ "7Z", 30, 186.200 },
	{ "--", 45, 189.900 },
	{ "7A", 31, 192.800 },
	{ "--", 46, 196.600 },
	{ "--", 47, 199.500 },
	{ "M1", 32, 203.500 },
	{ "8Z", 48, 206.500 },
	{ "M2", 33, 210.700 },
	{ "M3", 34, 218.100 },
	{ "M4", 35, 225.700 },
	{ "9Z", 49, 229.100 },
	{ "--", 36, 233.600 },
	{ "--", 37, 241.800 },
	{ "--", 38, 250.300 },
	{ "0Z", 50, 254.100 }
};

void ctcss_populate(OptionsField& field) {
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t ctcss_options;
	std::string f_string;
	uint32_t c, f;
	
	ctcss_options.emplace_back(std::make_pair("None", 0));
	for (c = 0; c < CTCSS_TONES_NB; c++) {
		f = (uint32_t)(ctcss_tones[c].frequency * 10);
		f_string = ctcss_tones[c].PL_code;
		f_string += " " + to_string_dec_uint(f / 10) + "." + to_string_dec_uint(f % 10);
		ctcss_options.emplace_back(f_string, c);
	}
	
	field.set_options(ctcss_options);
}

}
