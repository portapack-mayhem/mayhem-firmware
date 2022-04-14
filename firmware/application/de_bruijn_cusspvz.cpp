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
#include "string_format.hpp"

uint32_t de_bruijn::init(const uint8_t k_arg, const uint8_t n_arg)
{
	k = k_arg;
	n = n_arg;

	// calculate new length
	length = pow(k, n) + (n - 1);

	reset();

	return length;
}

void de_bruijn::reset()
{
	// fill the var `a` with 0s
	for (uint8_t i = 1; i <= sizeof(a); i++)
	{
		a[i] = 0;
	};

	t = 1;
	p = 1;
	o = 0;

	frame.clear();
}

void de_bruijn::compute()
{
	if (o > n)
		return;

	if (t > n)
		return feed_frame();

	a[t] = a[t - p];

	if (a[t] > 0)
		o++;

	t++;
	compute();

	uint32_t t_loop = t;
	// uint32_t p_loop = p;
	// uint32_t o_loop = o;
	for (int32_t j = a[t - p] + 1; j <= k - 1; j++)
	{
		a[t_loop] = j;
		t = t_loop + 1;
		p = t_loop;
		compute();
	}
}

void de_bruijn::feed_frame()
{
	if (n % p != 0)
		return;

	if (k != 2)
		return;

	for (uint32_t j = 1; j <= p; j++)
	{
		t = a[j];
		frame += to_string_dec_uint(t);
	};
}