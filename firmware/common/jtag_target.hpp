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

#ifndef __JTAG_TARGET_H__
#define __JTAG_TARGET_H__

#include <cstddef>
#include <cstdint>

namespace jtag {

class Target {
 public:
	using bit_t = uint_fast8_t;

	virtual ~Target() {}

	virtual void delay(const size_t n) = 0;
	virtual bit_t clock(const bit_t tms_value, const bit_t tdi_value) = 0;
};

} /* namespace jtag */

#endif /*__JTAG_TARGET_H__*/
