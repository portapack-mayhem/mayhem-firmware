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

#include "cpld_max5.hpp"

#include "jtag.hpp"

#include <cstdint>
#include <array>

namespace cpld {
namespace max5 {

void CPLD::bypass() {
	shift_ir(instruction_t::BYPASS);
	jtag.runtest_tck(18003);
}

void CPLD::sample() {
	shift_ir(instruction_t::SAMPLE);
	jtag.runtest_tck(93);
	for(size_t i=0; i<80; i++) {
		jtag.shift_dr(3, 0b111);
	}
}

void CPLD::sample(std::bitset<240>& value) {
	shift_ir(instruction_t::SAMPLE);
	jtag.runtest_tck(93);
	shift_dr(value);
}

void CPLD::extest(std::bitset<240>& value) {
	shift_ir(instruction_t::EXTEST);
	shift_dr(value);
}

void CPLD::clamp() {
	shift_ir(instruction_t::CLAMP);
	jtag.runtest_tck(93);
}

void CPLD::enable() {
	shift_ir(instruction_t::ISC_ENABLE);
	jtag.runtest_tck(18003);		// 1ms
}

void CPLD::disable() {
	shift_ir(instruction_t::ISC_DISABLE);
	jtag.runtest_tck(18003);		// 1ms
}

/* Sector erase:
 * Involves shifting in the instruction to erase the device and applying
 * an erase pulse or pulses. The erase pulse is automatically generated
 * internally by waiting in the run, test, or idle state for the
 * specified erase pulse time of 500 ms for the CFM block and 500 ms for
 * each sector of the user flash memory (UFM) block.
 */
void CPLD::bulk_erase() {
	erase_sector(0x0011);
	erase_sector(0x0001);
	erase_sector(0x0000);
}

bool CPLD::program(
	const std::array<uint16_t, 3328>& block_0,
	const std::array<uint16_t, 512>& block_1
) {
	bulk_erase();

	/* Program:
	 * involves shifting in the address, data, and program instruction and
	 * generating the program pulse to program the flash cells. The program
	 * pulse is automatically generated internally by waiting in the run/test/
	 * idle state for the specified program pulse time of 75 μs. This process
	 * is repeated for each address in the CFM and UFM blocks.
	 */
	program_block(0x0000, block_0);
	program_block(0x0001, block_1);

	const auto verify_ok = verify(block_0, block_1);

	if( verify_ok ) {
		/* Do "something". Not sure what, but it happens after verify. */
		/* Starts with a sequence the same as Program: Block 0. */
		/* Perhaps it is a write to tell the CPLD that the bitstream
		 * verified OK, and it's OK to load and execute? And despite only
		 * one bit changing, a write must be a multiple of a particular
		 * length (64 bits)? */
		sector_select(0x0000);
		shift_ir(instruction_t::ISC_PROGRAM);
		jtag.runtest_tck(93);		// 5 us

		/* TODO: Use data from cpld_block_0, with appropriate bit(s) changed */
		/* Perhaps this is the "ISP_DONE" bit? */
		jtag.shift_dr(16, block_0[0] & 0xfbff);
		jtag.runtest_tck(1800);		// 100us
		jtag.shift_dr(16, block_0[1]);
		jtag.runtest_tck(1800);		// 100us
		jtag.shift_dr(16, block_0[2]);
		jtag.runtest_tck(1800);		// 100us
		jtag.shift_dr(16, block_0[3]);
		jtag.runtest_tck(1800);		// 100us
	}

	return verify_ok;
}

bool CPLD::verify(
	const std::array<uint16_t, 3328>& block_0,
	const std::array<uint16_t, 512>& block_1
) {
	/* Verify */
	const auto block_0_success = verify_block(0x0000, block_0);
	const auto block_1_success = verify_block(0x0001, block_1);
	return block_0_success && block_1_success;
}

uint32_t CPLD::crc() {
	crc_t crc { 0x04c11db7, 0xffffffff, 0xffffffff };
	block_crc(0, 3328, crc);
	block_crc(1,  512, crc);
	return crc.checksum();
}

void CPLD::sector_select(const uint16_t id) {
	shift_ir(instruction_t::ISC_ADDRESS_SHIFT);
	jtag.runtest_tck(93);		// 5us
	jtag.shift_dr(13, id);		// Sector ID
}

bool CPLD::idcode_ok() {
	shift_ir(instruction_t::IDCODE);
	const auto idcode_read = jtag.shift_dr(idcode_length, 0);
	return (idcode_read == idcode);
}

std::array<uint16_t, 5> CPLD::read_silicon_id() {
	sector_select(0x0089);
	shift_ir(instruction_t::ISC_READ);
	jtag.runtest_tck(93);		// 5us

	std::array<uint16_t, 5> silicon_id;
	silicon_id[0] = jtag.shift_dr(16, 0xffff);
	silicon_id[1] = jtag.shift_dr(16, 0xffff);
	silicon_id[2] = jtag.shift_dr(16, 0xffff);
	silicon_id[3] = jtag.shift_dr(16, 0xffff);
	silicon_id[4] = jtag.shift_dr(16, 0xffff);
	return silicon_id;
}

/* Check ID:
 * The silicon ID is checked before any Program or Verify process. The
 * time required to read this silicon ID is relatively small compared to
 * the overall programming time.
 */
bool CPLD::silicon_id_ok() {
	const auto silicon_id = read_silicon_id();

	return (
		(silicon_id[0] == 0x8232) &&
		(silicon_id[1] == 0x2aa2) &&
		(silicon_id[2] == 0x4a82) &&
		(silicon_id[3] == 0x8c0c) &&
		(silicon_id[4] == 0x0000)
	);
}

uint32_t CPLD::usercode() {
	shift_ir(instruction_t::USERCODE);
	jtag.runtest_tck(93);	// 5us
	return jtag.shift_dr(32, 0xffffffff);
}

void CPLD::erase_sector(const uint16_t id) {
	sector_select(id);
	shift_ir(instruction_t::ISC_ERASE);
	jtag.runtest_tck(9000003);	// 500ms
}

void CPLD::program_block(
	const uint16_t id,
	const uint16_t* const data,
	const size_t count
) {
	sector_select(id);
	shift_ir(instruction_t::ISC_PROGRAM);
	jtag.runtest_tck(93);		// 5us

	for(size_t i=0; i<count; i++) {
		jtag.shift_dr(16, data[i]);
		jtag.runtest_tck(1800);
	}
}

bool CPLD::verify_block(
	const uint16_t id,
	const uint16_t* const data,
	const size_t count
) {
	sector_select(id);
	shift_ir(instruction_t::ISC_READ);
	jtag.runtest_tck(93);		// 5us

	bool success = true;
	for(size_t i=0; i<count; i++) {
		const auto from_device = jtag.shift_dr(16, 0xffff);
		if( from_device != data[i] ) {
			if( (id == 0) && (i == 0) ) {
				// Account for bit that indicates bitstream is valid.
				if( (from_device & 0xfbff) != (data[i] & 0xfbff) ) {
					success = false;
				}
			} else {
				success = false;
			}
		}
	}
	return success;
}

bool CPLD::is_blank_block(const uint16_t id, const size_t count) {
	sector_select(id);
	shift_ir(instruction_t::ISC_READ);
	jtag.runtest_tck(93);		// 5us

	bool success = true;
	for(size_t i=0; i<count; i++) {
		const auto from_device = jtag.shift_dr(16, 0xffff);
		if( from_device != 0xffff ) {
			success = false;
		}
	}
	return success;
}

void CPLD::block_crc(const uint16_t id, const size_t count, crc_t& crc) {
	sector_select(id);
	shift_ir(instruction_t::ISC_READ);
	jtag.runtest_tck(93);		// 5us

	for(size_t i=0; i<count; i++) {
		const uint16_t from_device = jtag.shift_dr(16, 0xffff);
		crc.process_bytes(&from_device, sizeof(from_device));
	}
}

bool CPLD::is_blank() {
	const auto block_0_blank = is_blank_block(0x0000, 3328);
	const auto block_1_blank = is_blank_block(0x0001, 512);
	return block_0_blank && block_1_blank;
}

} /* namespace max5 */
} /* namespace cpld */
