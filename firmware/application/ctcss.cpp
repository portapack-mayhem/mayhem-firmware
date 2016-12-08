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

ctcss_tone ctcss_tones[CTCSS_TONES_NB] = {
	{ "XZ", 0, 67000 },
	{ "WZ", 1, 69400 },
	{ "XA", 39, 71900 },
	{ "WA", 3, 74400 },
	{ "XB", 4, 77000 },
	{ "WB", 5, 79700 },
	{ "YZ", 6, 82500 },
	{ "YA", 7, 85400 },
	{ "YB", 8, 88500 },
	{ "ZZ", 9, 91500 },
	{ "ZA", 10, 94800 },
	{ "ZB", 11, 97400 },
	{ "1Z", 12, 100000 },
	{ "1A", 13, 103500 },
	{ "1B", 14, 107200 },
	{ "2Z", 15, 110900 },
	{ "2Z", 16, 114800 },
	{ "2B", 17, 118800 },
	{ "3Z", 18, 123000 },
	{ "3A", 19, 127300 },
	{ "3B", 20, 131800 },
	{ "4Z", 21, 136500 },
	{ "4A", 22, 141300 },
	{ "4B", 23, 146200 },
	{ "5Z", 24, 151400 },
	{ "5A", 25, 156700 },
	{ "--", 40, 159800 },
	{ "5B", 26, 162200 },
	{ "--", 41, 165500 },
	{ "6Z", 27, 167900 },
	{ "--", 42, 171300 },
	{ "6A", 28, 173800 },
	{ "--", 43, 177300 },
	{ "6B", 29, 179900 },
	{ "--", 44, 183500 },
	{ "7Z", 30, 186200 },
	{ "--", 45, 189900 },
	{ "7A", 31, 192800 },
	{ "--", 46, 196600 },
	{ "--", 47, 199500 },
	{ "M1", 32, 203500 },
	{ "8Z", 48, 206500 },
	{ "M2", 33, 210700 },
	{ "M3", 34, 218100 },
	{ "M4", 35, 225700 },
	{ "9Z", 49, 229100 },
	{ "--", 36, 233600 },
	{ "--", 37, 241800 },
	{ "--", 38, 250300 },
	{ "0Z", 50, 254100 }
};
