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

#include "de_bruijn_cusspvz.hpp"
#include <math.h> /* pow */

void de_bruijn::init(std::string alphabet_str, uint8_t wordlength, uint8_t bytes_per_part)
{
	alphabet = alphabet_str;
	k = alphabet_str.length();
	n = wordlength;
	_bytes_per_part = bytes_per_part;

	// calculate new length
	length = pow(k, n) + (n - 1);

	// calculate the number of parts
	std::div_t tmp = div(length, bytes_per_part);
	_total_parts = tmp.quot + (tmp.rem > 0) ? 1 : 0;

	reset();
}

void de_bruijn::reset()
{
	// fill the var `a` with 0s
	for (uint8_t i = 1; i <= sizeof(a); i++)
	{
		a[i] = 0;
	};

	sequence.clear();
}

void de_bruijn::db(uint8_t t, uint8_t p)
{
	if (t > n)
	{
		if (n % p == 0)
			for (uint32_t j = 1; j <= p; j++)
				sequence += alphabet[a[j]];

		return;
	}

	a[t] = a[t - p];
	db(t + 1, p);
	for (uint8_t j = a[t - p] + 1; j < k; j++)
	{
		a[t] = j;
		db(t + 1, t);
	}
}

void de_bruijn::generate()
{
	db(1, 1);

	for (uint8_t i = 0, nremain = n - 1; nremain > 0; i += 2, nremain--)
		sequence += sequence[i % sequence.length()];
}

uint32_t de_bruijn::get_total_parts()
{
	return _total_parts;
}

std::string de_bruijn::get_part(uint32_t part_index)
{
	if (part_index >= _total_parts)
	{
		return "";
	}

	return sequence.substr(part_index * _bytes_per_part, _bytes_per_part);
}
