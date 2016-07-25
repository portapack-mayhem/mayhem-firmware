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

#ifndef __JTAG_H__
#define __JTAG_H__

#include "jtag_target.hpp"

#include <cstdint>
#include <cstddef>

#include <bitset>

namespace jtag {

class JTAG {
public:
	constexpr JTAG(
		Target& target
	) : target(target)
	{
	}

	void reset() {
		/* ??? -> Test-Logic-Reset */
		for(size_t i=0; i<8; i++) {
			target.clock(1, 0);
		}
	}

	void run_test_idle() {
		/* Test-Logic-Reset -> Run-Test/Idle */
		target.clock(0, 0);
	}

	void runtest_tck(const size_t count) {
		target.delay(count);
	}

	uint32_t shift_ir(const size_t count, const uint32_t value) {
		/* Run-Test/Idle -> Select-DR-Scan -> Select-IR-Scan */
		target.clock(1, 0);
		target.clock(1, 0);
		/* Scan -> Capture -> Shift */
		target.clock(0, 0);
		target.clock(0, 0);

		const auto result = shift(count, value);

		/* Exit1 -> Update */
		target.clock(1, 0);
		/* Update -> Run-Test/Idle */
		target.clock(0, 0);

		return result;
	}

	uint32_t shift_dr(const size_t count, const uint32_t value) {
		/* Run-Test/Idle -> Select-DR-Scan */
		target.clock(1, 0);
		/* Scan -> Capture -> Shift */
		target.clock(0, 0);
		target.clock(0, 0);

		const auto result = shift(count, value);

		/* Exit1 -> Update */
		target.clock(1, 0);
		/* Update -> Run-Test/Idle */
		target.clock(0, 0);

		return result;
	}

private:
	Target& target;

	uint32_t shift(const size_t count, uint32_t value);
};

} /* namespace jtag */

#endif/*__JTAG_H__*/
