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

#ifndef __CPLD_XILINX_H__
#define __CPLD_XILINX_H__

#include "jtag_tap.hpp"

#include "utility.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

namespace cpld {
namespace xilinx {

using jtag::tap::state_t;

class XC2C64A {
public:
	using block_id_t = uint8_t;

	static constexpr size_t block_length  = 274;
	static constexpr size_t blocks_count = 98;

	static constexpr size_t block_bytes = (block_length + 7) >> 3;

	struct verify_block_t {
		block_id_t id;
		std::array<uint8_t, block_bytes> data;
		std::array<uint8_t, block_bytes> mask;
	};

	struct program_block_t {
		block_id_t id;
		std::array<uint8_t, block_bytes> data;
	};

	using verify_blocks_t = std::array<verify_block_t, blocks_count>;

	constexpr XC2C64A(
		jtag::Target& jtag_interface
	) : tap { jtag_interface }
	{
	}

	void write_sram(const verify_blocks_t& blocks);
	bool verify_sram(const verify_blocks_t& blocks);

	bool verify_eeprom(const verify_blocks_t& blocks);
	void init_from_eeprom();

private:
	static constexpr size_t idcode_length = 32;
	using idcode_t = uint32_t;

	static constexpr size_t ir_length = 8;
	static constexpr size_t block_id_length = 7;

	static constexpr idcode_t idcode      = 0x06e58093;
	static constexpr idcode_t idcode_mask = 0x0fff8fff;

	using ir_t = uint8_t;

	jtag::tap::TAPMachine tap;

	enum class instruction_t : ir_t {
		INTEST           = 0b00000010,	// -> boundary-scan
		BYPASS           = 0b11111111,	// -> bypass
		SAMPLE           = 0b00000011,	// -> boundary-scan
		EXTEST           = 0b00000000,	// -> boundary-scan
		IDCODE           = 0b00000001,	// -> device ID
		USERCODE         = 0b11111101,	// -> device ID
		HIGHZ            = 0b11111100,	// -> bypass
		ISC_ENABLE_CLAMP = 0b11101001,	// -> ISC shift
		ISC_ENABLE_OTF   = 0b11100100,	// -> ISC shift
		ISC_ENABLE       = 0b11101000,	// -> ISC shift
		ISC_SRAM_READ    = 0b11100111,	// -> ISC shift
		ISC_WRITE        = 0b11100110,	// -> ISC shift, alias ISC_SRAM_WRITE
		ISC_ERASE        = 0b11101101,	// -> ISC shift
		ISC_PROGRAM      = 0b11101010,	// -> ISC shift
		ISC_READ         = 0b11101110,	// -> ISC shift, alias ISC_VERIFY
		ISC_INIT         = 0b11110000,	// -> ISC shift
		ISC_DISABLE      = 0b11000000,	// -> ISC shift
		TEST_ENABLE      = 0b00010001,	// alias Private1
		BULKPROG         = 0b00010010,	// alias Private2
		ERASE_ALL        = 0b00010100,	// alias Private4
		MVERIFY          = 0b00010011,	// alias Private3
		TEST_DISABLE     = 0b00010101,	// alias Private5
		ISC_NOOP         = 0b11100000,	// -> bypass
	};

	bool shift_ir(const instruction_t instruction);

	void reset();
	void enable();
	void enable_otf();
	void discharge();
	void init();
	void disable();
	bool bypass();
};

} /* namespace xilinx */
} /* namespace cpld */

#endif/*__CPLD_XILINX_H__*/
