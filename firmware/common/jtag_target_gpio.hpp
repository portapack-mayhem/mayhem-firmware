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

#ifndef __JTAG_TARGET_GPIO_H__
#define __JTAG_TARGET_GPIO_H__

#include "jtag_target.hpp"

#include <ch.h>

#include <cstddef>

namespace jtag {

class GPIOTarget : public Target {
public:
	// Tightly control how this object can be constructor or copied, since
	// it initializes GPIOs in the constructor. I don't want it re-
	// initializing a bunch as the object gets passed around. So don't let
	// it get passed around...
	/*
	GPIOTarget() = delete;
	GPIOTarget(const GPIOTarget&) = delete;
	GPIOTarget(GPIOTarget&&) = delete;
	GPIOTarget& operator=(const GPIOTarget&) = delete;
	GPIOTarget& operator=(GPIOTarget&&) = delete;
	*/
	GPIOTarget(
		GPIO gpio_tck,
		GPIO gpio_tms,
		GPIO gpio_tdi,
		GPIO gpio_tdo
	) : gpio_tck { gpio_tck },
		gpio_tms { gpio_tms },
		gpio_tdi { gpio_tdi },
		gpio_tdo { gpio_tdo }
	{
		gpio_tdi.write(1);
		gpio_tdi.output();
		gpio_tdi.configure();

		gpio_tms.write(1);
		gpio_tms.output();
		gpio_tms.configure();

		gpio_tck.write(0);
		gpio_tck.output();
		gpio_tck.configure();

		gpio_tdo.input();
		gpio_tdo.configure();
	}

	~GPIOTarget() {
		gpio_tdi.input();
		gpio_tms.input();
		gpio_tck.input();
	}

	void delay(const size_t clocks) override {
		/* TODO: Use more precise timing mechanism, using the frequency
		 * specified by SVF file.
		 */
        halPolledDelay(clocks * systicks_per_tck);
	}

	Target::bit_t clock(
		const Target::bit_t tms_value,
		const Target::bit_t tdi_value
	) override {
		/* TODO: Use more precise timing mechanism, using the frequency
		 * specified by SVF file.
		 */
		const auto result = tdo();
		tms(tms_value);
		tdi(tdi_value);
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		tck(1);
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		__asm__("nop");
		tck(0);
		return result;
	}

private:
	/* At 200MHz, one 18MHz cycle is 11 systicks. */
	static constexpr size_t systicks_per_tck = 11;

	GPIO gpio_tck;
	GPIO gpio_tms;
	GPIO gpio_tdi;
	GPIO gpio_tdo;

	void tck(const Target::bit_t value) {
		gpio_tck.write(value);
	}

	void tdi(const Target::bit_t value) {
		gpio_tdi.write(value);
	}

	void tms(const bit_t value) {
		gpio_tms.write(value);
	}

	Target::bit_t tdo() {
		return gpio_tdo.read();
	}
};

} /* namespace jtag */

#endif/*__JTAG_TARGET_GPIO_H__*/
