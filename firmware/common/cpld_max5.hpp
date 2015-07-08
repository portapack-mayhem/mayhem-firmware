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

#ifndef __CPLD_MAX5_H__
#define __CPLD_MAX5_H__

#include "jtag.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <bitset>

namespace cpld {
namespace max5 {

class CPLD {
public:
	constexpr CPLD(
		jtag::JTAG& jtag
	) : jtag(jtag)
	{
	}

	void reset() {
		jtag.reset();
	}

	void run_test_idle() {
		jtag.run_test_idle();
	}

	bool idcode_ok();

	void enter_isp();

	/* Check ID:
	 * The silicon ID is checked before any Program or Verify process. The
	 * time required to read this silicon ID is relatively small compared to
	 * the overall programming time.
	 */
	bool silicon_id_ok();

	void exit_isp();	// I think that's what the code does...

	void bulk_erase();

	bool program(
		const std::array<uint16_t, 3328>& block_0,
		const std::array<uint16_t, 512>& block_1
	);

	bool verify(
		const std::array<uint16_t, 3328>& block_0,
		const std::array<uint16_t, 512>& block_1
	);

	bool is_blank();

	std::pair<bool, uint8_t> boundary_scan();

	enum class Instruction {
		BYPASS = 0b1111111111,
		EXTEST = 0b0000001111,
		SAMPLE = 0b0000000101,
		IDCODE = 0b0000000110,
		USERCODE = 0b0000000111,
		CLAMP = 0b0000001010,
		HIGHZ = 0b0000001011
	};

	void shift_ir(const Instruction instruction) {
		shift_ir(static_cast<uint32_t>(instruction));
	}

	template<size_t N>
	void shift_dr(std::bitset<N>& bits) {
		jtag.shift_dr(bits);
	}

private:
	jtag::JTAG& jtag;

	std::array<uint16_t, 5> read_silicon_id();

	void sector_select(const uint16_t id);

	void erase_sector(const uint16_t id);

	void program_block(
		const uint16_t id,
		const uint16_t* const data,
		const size_t count
	);

	template<size_t N>
	void program_block(
		const uint16_t id,
		const std::array<uint16_t, N>& data
	) {
		program_block(id, data.data(), data.size());
	}

	bool verify_block(
		const uint16_t id,
		const uint16_t* const data,
		const size_t count
	);

	template<size_t N>
	bool verify_block(
		const uint16_t id,
		const std::array<uint16_t, N>& data
	) {
		return verify_block(id, data.data(), data.size());
	}

	bool is_blank_block(const uint16_t id, const size_t count);

	const uint32_t IDCODE = 0b00000010000010100101000011011101;

	const size_t IR_LENGTH = 10;

	void shift_ir(const uint32_t value) {
		jtag.shift_ir(IR_LENGTH, value);
	}
};
/*
class ModeISP {
public:
	ModeISP(
		CPLD& cpld
	) : cpld(cpld)
	{
		cpld.enter_isp();
	}

	~ModeISP() {
		cpld.exit_isp();
	}

private:
	CPLD& cpld;
};
*/
} /* namespace max5 */
} /* namespace cpld */

#endif/*__CPLD_MAX5_H__*/
