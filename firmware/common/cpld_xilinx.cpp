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

#include "cpld_xilinx.hpp"

namespace cpld {
namespace xilinx {

void XC2C64A::write_sram(const verify_blocks_t& blocks) {
	tap.set_repeat(0);
	tap.set_end_ir(state_t::run_test_idle);
	tap.set_end_dr(state_t::run_test_idle);

	reset();
	enable();

	shift_ir(instruction_t::ISC_WRITE);
	for(const auto& block : blocks) {
		tap.state(state_t::shift_dr);
		tap.shift({ block.data.data(), block_length }, false);
		tap.shift({ &block.id, block_id_length }, true);
		tap.state(state_t::run_test_idle);
	}

	disable();
	bypass();

	tap.state(state_t::test_logic_reset);
}

bool XC2C64A::verify_sram(const verify_blocks_t& blocks) {
	tap.set_repeat(0);
	tap.set_end_ir(state_t::run_test_idle);
	tap.set_end_dr(state_t::run_test_idle);

	reset();
	enable();

	shift_ir(instruction_t::ISC_SRAM_READ);

	// Prime the operation with a read of an empty row.
	const jtag::tap::bits_t empty_row { block_length };

	tap.state(state_t::shift_dr);
	tap.shift(empty_row, false);
	
	auto error = false;
	for(const auto& block : blocks) {
		tap.shift({ &block.id, block_id_length }, true);
		tap.state(state_t::run_test_idle);
		
		tap.state(state_t::shift_dr);
		error |= tap.shift(empty_row, { block.data.data(), block_length }, { block.mask.data(), block_length }, false);
	}
	// Redundant operation to finish the row.
	tap.shift({ &blocks[0].id, block_id_length }, true);
	tap.state(state_t::run_test_idle);
	tap.set_end_dr(state_t::run_test_idle);

	disable();
	bypass();

	tap.state(state_t::test_logic_reset);

	return !error;
}

bool XC2C64A::verify_eeprom(const verify_blocks_t& blocks) {
	tap.set_repeat(0);
	tap.set_end_ir(state_t::run_test_idle);
	tap.set_end_dr(state_t::run_test_idle);

	reset();
	bypass();
	enable();

	shift_ir(instruction_t::ISC_READ);

	const jtag::tap::bits_t empty_row { block_length };

	auto error = false;
	for(const auto& block : blocks) {
		tap.set_end_dr(state_t::pause_dr);
		tap.shift_dr({ &block.id, block_id_length });
		tap.set_end_ir(state_t::run_test_idle);
		tap.wait(state_t::pause_dr, state_t::pause_dr, 20);
		tap.set_end_ir(state_t::run_test_idle);
		tap.wait(state_t::run_test_idle, state_t::run_test_idle, 100);
		error |= tap.shift_dr(empty_row, { block.data.data(), block_length }, { block.mask.data(), block_length });
		tap.wait(state_t::run_test_idle, state_t::run_test_idle, 100);
	}

	disable();
	bypass();

	tap.state(state_t::test_logic_reset);

	return !error;
}

void XC2C64A::init_from_eeprom() {
	tap.set_repeat(0);
	tap.set_end_ir(state_t::run_test_idle);
	tap.set_end_dr(state_t::run_test_idle);

	reset();
	enable();

	discharge();
	init();
	
	disable();
	bypass();

	tap.state(state_t::test_logic_reset);
}

bool XC2C64A::shift_ir(const instruction_t instruction) {
	const ir_t ir_buffer = toUType(instruction);
	const jtag::tap::bits_t bits { &ir_buffer, ir_length };
	return tap.shift_ir(bits);
}

void XC2C64A::reset() {
	tap.state(state_t::test_logic_reset);
	tap.state(state_t::run_test_idle);
}

void XC2C64A::enable() {
	shift_ir(instruction_t::ISC_ENABLE);
	tap.wait(state_t::run_test_idle, state_t::run_test_idle, 800);
}

void XC2C64A::enable_otf() {
	shift_ir(instruction_t::ISC_ENABLE_OTF);
}

void XC2C64A::discharge() {
	shift_ir(instruction_t::ISC_INIT);
	tap.wait(state_t::run_test_idle, state_t::run_test_idle, 20);
}

void XC2C64A::init() {
	tap.set_end_ir(state_t::update_ir);
	shift_ir(instruction_t::ISC_INIT);
	tap.set_end_ir(state_t::run_test_idle);
	tap.state(state_t::capture_dr);
	tap.wait(state_t::run_test_idle, state_t::run_test_idle, 800);
}

void XC2C64A::disable() {
	shift_ir(instruction_t::ISC_DISABLE);
	tap.wait(state_t::run_test_idle, state_t::run_test_idle, 100);
}

bool XC2C64A::bypass() {
	return shift_ir(instruction_t::BYPASS);
}

} /* namespace xilinx */
} /* namespace cpld */
