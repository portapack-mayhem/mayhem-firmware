/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UNITS_H__
#define __UNITS_H__

#include <cstdint>

namespace units {

class Pressure {
public:
	constexpr Pressure(
	) : kpa_ { 0 }
	{
	}

	constexpr Pressure(
		const int kilopascal
	) : kpa_ { static_cast<int16_t>(kilopascal) }
	{
	}

	int kilopascal() const {
		return kpa_;
	}

	int psi() const {
		return kpa_ * 1000 / 6895;
	}

private:
	int16_t kpa_;
};

class Temperature {
public:
	constexpr Temperature(
	) : c_ { 0 }
	{
	}

	constexpr Temperature(
		const int celsius
	) : c_ { static_cast<int16_t>(celsius) }
	{
	}

	int celsius() const {
		return c_;
	}

	int fahrenheit() const {
		return (c_ * 9 / 5) + 32;
	}

private:
	int16_t c_;
};

} /* namespace units */

#endif/*__UNITS_H__*/
