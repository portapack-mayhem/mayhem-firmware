/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <cstdint>
#include <cstddef>

template<typename T>
struct buffer_t {
	T* const p;
	const size_t count;
	const uint32_t sampling_rate;

	constexpr buffer_t(
		T* const p,
		const size_t count
	) : p { p },
		count { count },
		sampling_rate { 0 }
	{
	};

	constexpr buffer_t(
		T* const p,
		const size_t count,
		const uint32_t sampling_rate
	) : p { p },
		count { count },
		sampling_rate { sampling_rate }
	{
	};
};

#endif/*__BUFFER_H__*/
