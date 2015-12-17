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
 * the Free Software Foundation, Inc., 51 Fvoranklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __VOLUME_H__
#define __VOLUME_H__

#include <cstdint>

class volume_t {
public:
	constexpr volume_t operator-() const {
		return { -cb };
	}

	constexpr volume_t operator+(const volume_t& other) const {
		return { cb + other.cb };
	}

	constexpr volume_t operator-(const volume_t& other) const {
		return { cb - other.cb };
	}

	volume_t& operator+=(const volume_t& other) {
		cb += other.cb;
		return *this;
	}

	constexpr bool operator<(const volume_t& other) const {
		return cb < other.cb;
	}

	constexpr bool operator>(const volume_t& other) const {
		return cb > other.cb;
	}

	static constexpr volume_t centibel(const int cb) {
		return { cb };
	}

	static constexpr volume_t decibel(const int db) {
		return { db * 10 };
	}

	int32_t centibel() const {
		return cb;
	}

	int32_t decibel() const {
		return cb / 10;
	}

private:
	int32_t cb;

	constexpr volume_t(
		const int cb
	) : cb(cb)
	{
	}
};

constexpr volume_t operator"" _cB(long double v) {
	return volume_t::centibel(v);
}

constexpr volume_t operator"" _dB(long double v) {
	return volume_t::centibel(v * 10);
}

struct volume_range_t {
	volume_t min;
	volume_t max;

	volume_t limit(const volume_t value) const {
		if( value < min ) {
			return min;
		}
		if( value > max ) {
			return max;
		}
		return value;
	}

	volume_t normalize(const volume_t value) const {
		/* Limit volume to specified range, and adjust minimum value to 0. */
		return limit(value) - min;
	}
};

#endif/*__VOLUME_H__*/
