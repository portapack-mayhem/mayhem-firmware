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

#ifndef __MAX5864_H__
#define __MAX5864_H__

#include "spi_arbiter.hpp"

namespace max5864 {

enum class Mode : uint8_t {
	Shutdown	= 0x00,
	Idle		= 0x01,
	Receive		= 0x02,
	Transmit	= 0x03,
	Transceiver	= 0x04,
	Standby		= 0x05,
};

class MAX5864 {
public:
	constexpr MAX5864(
		spi::arbiter::Target& target
	) : _target(target)
	{
	}

	void init() {
		/* Shut down explicitly, as there is no other reset mechanism. */
		set_mode(Mode::Shutdown);
	}

	void set_mode(const Mode mode);

private:
	spi::arbiter::Target& _target;
};

}

#endif/*__MAX5864_H__*/
