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

struct de_bruijn
{
public:
	uint32_t init(const uint8_t k_arg, const uint8_t n_arg); // returns length
	void reset();
	void compute();

	std::string frame = "";
	uint32_t length = 0;

	uint8_t k = 2; // radix
	uint8_t n = 8; // data length
private:
	void feed_frame();

	uint32_t t = 1;
	uint32_t p = 1;
	uint32_t o = 0;
	uint8_t a[101] = {};
};

#endif /*__DE_BRUIJN_H__*/
