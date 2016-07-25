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

#ifndef __JTAG_TAP_H__
#define __JTAG_TAP_H__

#include "jtag_target.hpp"

#include <cstdint>
#include <cstddef>

namespace jtag {
namespace tap {

class bits_t {
public:
	constexpr bits_t(
	) : p { nullptr },
		count { 0 }
	{
	}

	constexpr bits_t(
		const size_t count,
		const bool default_value = true
	) : p { nullptr },
		count { count },
		default_value { default_value }
	{
	}

	constexpr bits_t(
		const uint8_t* const p,
		const size_t count
	) : p { p },
		count { count }
	{
	}

	size_t length() const;

	explicit operator bool() const;

	bool operator[](const size_t index) const;

private:
	const uint8_t* p { nullptr };
	size_t count { 0 };
	bool default_value { false };

	size_t bytes() const;
};

enum class state_t : uint8_t {
	/* Ordinal values are important for "routes" values, and to match XSVF definitions. */
	test_logic_reset = 0,
	run_test_idle = 1,
	select_dr_scan = 2,
	capture_dr = 3,
	shift_dr = 4,
	exit1_dr = 5,
	pause_dr = 6,
	exit2_dr = 7,
	update_dr = 8,
	select_ir_scan = 9,
	capture_ir = 10,
	shift_ir = 11,
	exit1_ir = 12,
	pause_ir = 13,
	exit2_ir = 14,
	update_ir = 15,
};

class TAPState {
public:
	constexpr TAPState(
	) : _state { state_t::test_logic_reset }
	{
	}

	state_t state() const;
	void advance(const bool tms);
	bool advance_toward(const state_t desired_state) const;

private:
	state_t _state;
};

class TAPMachine {
public:
	constexpr TAPMachine(
		jtag::Target& target
	) : target { target }
	{
	}

	void set_run_test(const uint32_t value);
	void set_repeat(const uint8_t value);
	void set_end_ir(const state_t state);
	void set_end_dr(const state_t state);

	bool shift(const bits_t& tdi, const bool end_tms) {
		return shift(tdi, {}, {}, end_tms);
	}
	
	bool shift(const bits_t& tdi, const bits_t& tdo_expected, const bits_t& tdo_mask, const bool end_tms);

	bool shift_ir(const bits_t& tdi_value, const bits_t& tdo_expected = {}, const bits_t& tdo_mask = {});
	bool shift_dr(const bits_t& tdi_value, const bits_t& tdo_expected = {}, const bits_t& tdo_mask = {});

	void state(const state_t state);
	
	void wait(const state_t wait_state, const state_t end_state, const uint32_t wait_time);

private:
	jtag::Target& target;
	TAPState tap { };

	uint32_t _run_test { 0 };
	uint8_t _repeat { 0 };
	state_t _end_ir { };
	state_t _end_dr { };

	bool clock(const bool tms, const bool tdi=false);
	void advance_to_state(const state_t desired_state);
	void delay_us(const uint32_t microseconds);

	void shift_start(const state_t state);
	void shift_end(const state_t end_state, const uint32_t end_delay);

	bool shift_data(const bits_t& tdi, const bits_t& tdo_expected, const bits_t& tdo_mask, const state_t state, const state_t end_state, const uint32_t end_delay);
};

} /* namespace tap */
} /* namespace jtag */

#endif/*__JTAG_TAP_H__*/
