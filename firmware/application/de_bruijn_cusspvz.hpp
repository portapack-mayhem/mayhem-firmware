/*
 * Copyright (C) 2022 José Moreira @cusspvz
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

#include <string>

#ifndef __DE_BRUIJN_H__
#define __DE_BRUIJN_H__

class de_bruijn
{
public:
	void init(const std::string alphabet_str, const uint8_t wordlength, const uint8_t bytes_per_part);
	void reset();
	void db(uint8_t t, uint8_t p);
	void generate();

	std::string alphabet = "";
	std::string sequence = "";
	uint32_t length = 0;

	uint8_t k = 2; // radix
	uint8_t n = 8; // data length

	// parts related - might be useful to distribute the computing
	// into a stream later down the road
	std::string
	get_part(uint32_t part_index);
	uint32_t get_total_parts();

private:
	void feed_sequence();
	uint8_t a[101] = {};
	uint32_t _total_parts;
	uint8_t _bytes_per_part;
};

#endif /*__DE_BRUIJN_H__*/
