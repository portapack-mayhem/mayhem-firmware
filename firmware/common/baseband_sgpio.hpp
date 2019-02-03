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

#ifndef __BASEBAND_GPIO_H__
#define __BASEBAND_GPIO_H__

#include <cstdint>

#include "baseband.hpp"

#include "hal.h"

namespace baseband {

class SGPIO {
public:
	void init();

	void configure(const Direction direction);

	void streaming_enable() {
		/* TODO: Any reason not to control from general GPIO facility? */
		LPC_SGPIO->GPIO_OUTREG &= ~(1U << 10);
	}

	void streaming_disable() {
		/* TODO: Any reason not to control from general GPIO facility? */
		LPC_SGPIO->GPIO_OUTREG |= (1U << 10);
	}

	bool streaming_is_enabled() const {
		/* TODO: Any reason not to control from general GPIO facility? */
		return (LPC_SGPIO->GPIO_OUTREG >> 10) & 1;
	}

private:
	void disable_all_slice_counters() {
		set_slice_counter_enables(0);
	}

	void set_slice_counter_enables(const uint16_t enable_mask) {
		LPC_SGPIO->CTRL_ENABLE = enable_mask;
	}
};

}

#endif/*__BASEBAND_GPIO_H__*/
