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

/* LPC43xx RTC structure. Avoiding using the ChibiOS-defined structure because
 * it pulls in all sorts of dependencies and initialization and other stuff that
 * the M0 needs to remain in control of.
 *
 * But yes, this is a hack, and something better is needed. It's too tangled of
 * a knot to tackle at the moment, though...
 */
#if defined(LPC43XX_M4)
struct Timestamp {
	uint32_t tv_date { 0 };
	uint32_t tv_time { 0 };

	static Timestamp now();
};
#endif

#if defined(LPC43XX_M0)
#include "lpc43xx_cpp.hpp"

using Timestamp = lpc43xx::rtc::RTC;
#endif

template<typename T>
struct buffer_t {
	T* const p;
	const size_t count;
	const uint32_t sampling_rate;
	const Timestamp timestamp;

	constexpr buffer_t(
	) : p { nullptr },
		count { 0 },
		sampling_rate { 0 },
		timestamp { }
	{
	}

	constexpr buffer_t(
		const buffer_t<T>& other
	) : p { other.p },
		count { other.count },
		sampling_rate { other.sampling_rate },
		timestamp { other.timestamp }
	{
	}

	constexpr buffer_t(
		T* const p,
		const size_t count,
		const uint32_t sampling_rate = 0,
		const Timestamp timestamp = { }
	) : p { p },
		count { count },
		sampling_rate { sampling_rate },
		timestamp { timestamp }
	{
	}

	operator bool() const {
		return (p != nullptr);
	}
};

#endif/*__BUFFER_H__*/
