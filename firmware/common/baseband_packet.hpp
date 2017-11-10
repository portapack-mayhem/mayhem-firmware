/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __BASEBAND_PACKET_H__
#define __BASEBAND_PACKET_H__

#include "baseband.hpp"

#include <cstddef>
#include <bitset>

namespace baseband {

class Packet {
public:
	void set_timestamp(const Timestamp& value) {
		timestamp_ = value;
	}
	
	Timestamp timestamp() const {
		return timestamp_;
	}

	void add(const bool symbol) {
		if( count < capacity() ) {
			data[count++] = symbol;
		}
	}

	uint_fast8_t operator[](const size_t index) const {
		return (index < size()) ? data[index] : 0;
	}

	size_t size() const {
		return count;
	}

	size_t capacity() const {
		return data.size();
	}

	void clear() {
		count = 0;
	}

private:
	std::bitset<2560> data { };
	Timestamp timestamp_ { };
	size_t count { 0 };
};

} /* namespace baseband */

#endif/*__BASEBAND_PACKET_H__*/
