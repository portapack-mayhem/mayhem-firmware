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

#include "jtag_tap.hpp"

#include "utility.hpp"

#include <string>

namespace jtag {
namespace tap {

size_t bits_t::length() const {
	return count;
}

bits_t::operator bool() const {
	return (count > 0);
}

bool bits_t::operator[](const size_t index) const {
	if( p && (index < count) ) {
		const auto n = bytes() * 8 - index - 1;
		const auto byte = n >> 3;
		const auto bit = (n ^ 7) & 7;
		return (p[byte] >> bit) & 1;
	} else {
		return default_value;
	}
}

size_t bits_t::bytes() const {
	return (count + 7) >> 3;
}

#if JTAG_TAP_DEBUG
static char nibble_to_hex_char(const uint8_t v) {
	if( v < 0xa ) {
		return 0x30 + v;
	} else {
		return 0x57 + v;
	}
}

std::string to_string(const bits_t& bits) {
	std::string s { std::to_string(bits.length()) + "/0x" };
	for(size_t i=0; i<bytes(); i++) {
		s += nibble_to_hex_char((bits.p[i] >> 4) & 0xf);
		s += nibble_to_hex_char((bits.p[i] >> 0) & 0xf);
	}
	return s;
}
#endif

namespace {

using route_t = uint16_t;

struct entry_t {
	state_t tms[2];
	route_t route;
};
static_assert(sizeof(entry_t) == 4, "Unexpected size of entry_t");

using table_t = std::array<entry_t, 16>;
static_assert(sizeof(table_t) == (16 * 4), "Unexpected size of table_t");

const table_t table { {
	{ state_t::run_test_idle,	state_t::test_logic_reset,	0b0000000000000001 },	// test_logic_reset	test_logic_reset => 1, others => 0
	{ state_t::run_test_idle,	state_t::select_dr_scan,	0b1111111111111101 },	// run_test_idle	run_test_idle => 0, others => 1
	{ state_t::capture_dr,		state_t::select_ir_scan,	0b1111111000000001 },	// select_dr_scan	run_test_idle..update_dr => 0, others => 1
	{ state_t::shift_dr,		state_t::exit1_dr,			0b1111111111101111 },	// capture_dr		shift_dr => 0, others => 1
	{ state_t::shift_dr,		state_t::exit1_dr,			0b1111111111101111 },	// shift_dr			shift_dr => 0, others => 1
	{ state_t::pause_dr,		state_t::update_dr,			0b1111111100111111 },	// exit1_dr			pause_dr, exit2_dr => 0, others => 1
	{ state_t::pause_dr,		state_t::exit2_dr,			0b1111111110111111 },	// pause_dr			pause_dr => 0, others => 1
	{ state_t::shift_dr,		state_t::update_dr,			0b1111111111101111 },	// exit2_dr			shift_dr => 0, others => 1
	{ state_t::run_test_idle, 	state_t::select_dr_scan,	0b1111111111111101 },	// update_dr		run_test_idle => 0, others => 1
	{ state_t::capture_ir,		state_t::test_logic_reset,	0b0000000000000001 },	// select_ir_scan	test_logic_reset => 1, others => 0
	{ state_t::shift_ir,		state_t::exit1_ir,			0b1111011111111111 },	// capture_ir		shift_ir => 0, others => 1
	{ state_t::shift_ir,		state_t::exit1_ir,			0b1111011111111111 },	// shift_ir			shift_ir => 0, others => 1
	{ state_t::pause_ir,		state_t::update_ir,			0b1001111111111111 },	// exit1_ir			pause_ir, exit2_ir => 0, others => 1
	{ state_t::pause_ir,		state_t::exit2_ir,			0b1101111111111111 },	// pause_ir			pause_ir => 0, others => 1
	{ state_t::shift_ir,		state_t::update_ir,			0b1111011111111111 },	// exit2_ir			shift_ir => 0, others => 1
	{ state_t::run_test_idle,	state_t::select_dr_scan,	0b1111111111111101 },	// update_ir		run_test_idle => 0, others => 1
} };

const std::array<const char*, 16> state_name { {
	"test_logic_reset",
	"run_test_idle",
	"select_dr_scan",
	"capture_dr",
	"shift_dr",
	"exit1_dr",
	"pause_dr",
	"exit2_dr",
	"update_dr",
	"select_ir_scan",
	"capture_ir",
	"shift_ir",
	"exit1_ir",
	"pause_ir",
	"exit2_ir",
	"update_ir",
} };

const std::array<const char*, 16> state_long_name { {
	"Test-Logic-Reset",
	"Run-Test/Idle",
	"Select-DR-Scan",
	"Capture-DR",
	"Shift-DR",
	"Exit1-DR",
	"Pause-DR",
	"Exit2-DR",
	"Update-DR",
	"Select-IR-Scan",
	"Capture-IR",
	"Shift-IR",
	"Exit1-IR",
	"Pause-IR",
	"Exit2-IR",
	"Update-IR",
} };

const entry_t& entry(const state_t state) {
	return table[toUType(state)];
}

} /* namespace */

const char* c_str(const state_t state) {
	return state_name[toUType(state)];
}

/* TAPState **************************************************************/

state_t TAPState::state() const {
	return _state;
}

void TAPState::advance(const bool tms) {
	_state = entry(_state).tms[tms];
}

bool TAPState::advance_toward(const state_t desired_state) const {
	return (entry(_state).route >> toUType(desired_state)) & 1;
}

/* TAPMachine ************************************************************/

void TAPMachine::set_run_test(const uint32_t value) {
	_run_test = value;
}

void TAPMachine::set_repeat(const uint8_t value) {
	_repeat = value;
}

void TAPMachine::set_end_ir(const state_t state) {
	_end_ir = state;
}

void TAPMachine::set_end_dr(const state_t state) {
	_end_dr = state;
}

bool TAPMachine::shift_ir(const bits_t& tdi_value, const bits_t& tdo_expected, const bits_t& tdo_mask) {
	return shift_data(tdi_value, tdo_expected, tdo_mask, state_t::shift_ir, _end_ir, _run_test);
}

bool TAPMachine::shift_dr(const bits_t& tdi_value, const bits_t& tdo_expected, const bits_t& tdo_mask) {
	return shift_data(tdi_value, tdo_expected, tdo_mask, state_t::shift_dr, _end_dr, _run_test);
}

void TAPMachine::state(const state_t state) {
	if( state == state_t::test_logic_reset ) {
		for(int i=0; i<5; i++) {
			clock(1);
		}
	} else {
		advance_to_state(state);
	}
}

void TAPMachine::wait(const state_t wait_state, const state_t end_state, const uint32_t wait_time) {
	advance_to_state(wait_state);
	delay_us(wait_time);
	advance_to_state(end_state);
}

bool TAPMachine::clock(const bool tms, const bool tdi) {
	tap.advance(tms);
	return target.clock(tms, tdi);
}

void TAPMachine::advance_to_state(const state_t desired_state) {
	while( tap.state() != desired_state ) {
		const auto tms = tap.advance_toward(desired_state);
		clock(tms);
	}
}

void TAPMachine::delay_us(const uint32_t microseconds) {
	target.delay(microseconds);
}

void TAPMachine::shift_start(const state_t state) {
	advance_to_state(state);
}

bool TAPMachine::shift(const bits_t& tdi, const bits_t& tdo_expected, const bits_t& tdo_mask, const bool end_tms) {
	if( tdo_expected.length() != tdo_mask.length() ) {
		return false;
	}
	if( tdo_expected && (tdi.length() != tdo_expected.length()) ) {
		return false;
	}

	auto tdo_error = false;
	for(uint32_t i=0; i<tdi.length(); i++) {
		const auto tms = end_tms & (i == (tdi.length() - 1));
		const auto tdo = clock(tms, tdi[i]);
		if( tdo_expected && tdo_mask ) {
			tdo_error |= (tdo & tdo_mask[i]) != (tdo_expected[i] & tdo_mask[i]);
		}
	}

	return tdo_error;
}

void TAPMachine::shift_end(const state_t end_state, const uint32_t end_delay) {
	if( end_delay ) {
		advance_to_state(state_t::run_test_idle);
		delay_us(end_delay);
	} else {
		advance_to_state(end_state);
	}
}

bool TAPMachine::shift_data(const bits_t& tdi, const bits_t& tdo_expected, const bits_t& tdo_mask, const state_t state, const state_t end_state, const uint32_t end_delay) {
	shift_start(state);
	const auto result = shift(tdi, tdo_expected, tdo_mask, true);
	shift_end(end_state, end_delay);
	return result;
}

} /* namespace tap */
} /* namespace jtag */
