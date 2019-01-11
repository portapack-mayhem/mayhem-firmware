/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "cpld_update.hpp"

#include "hackrf_gpio.hpp"
#include "portapack_hal.hpp"

#include "jtag_target_gpio.hpp"
#include "cpld_max5.hpp"
#include "cpld_xilinx.hpp"
#include "portapack_cpld_data.hpp"
#include "hackrf_cpld_data.hpp"

namespace portapack {
namespace cpld {

bool update_if_necessary(
	const Config config
) {
	jtag::GPIOTarget target {
		portapack::gpio_cpld_tck,
		portapack::gpio_cpld_tms,
		portapack::gpio_cpld_tdi,
		portapack::gpio_cpld_tdo
	};
	jtag::JTAG jtag { target };
	CPLD cpld { jtag };

	/* Unknown state */
	cpld.reset();
	cpld.run_test_idle();

	/* Run-Test/Idle */
	if( !cpld.idcode_ok() ) {
		return false;
	}

	cpld.sample();
	cpld.bypass();
	cpld.enable();

	/* If silicon ID doesn't match, there's a serious problem. Leave CPLD
	 * in passive state.
	 */
	if( !cpld.silicon_id_ok() ) {
		return false;
	}

	/* Verify CPLD contents against current bitstream. */
	auto ok = cpld.verify(config.block_0, config.block_1);

	/* CPLD verifies incorrectly. Erase and program with current bitstream. */
	if( !ok ) {
		ok = cpld.program(config.block_0, config.block_1);
	}

	/* If programming OK, reset CPLD to user mode. Otherwise leave it in
	 * passive (ISP) state.
	 */
	if( ok ) {
		cpld.disable();
		cpld.bypass();

		/* Initiate SRAM reload from flash we just programmed. */
		cpld.sample();
		cpld.clamp();
		cpld.disable();
	}

	return ok;
}

} /* namespace cpld */
} /* namespace portapack */

namespace hackrf {
namespace cpld {

static jtag::GPIOTarget jtag_target_hackrf() {
	return {
		hackrf::one::gpio_cpld_tck,
		hackrf::one::gpio_cpld_tms,
		hackrf::one::gpio_cpld_tdi,
		hackrf::one::gpio_cpld_tdo,
	};
}

bool load_sram() {
	auto jtag_target_hackrf_cpld = jtag_target_hackrf();
	hackrf::one::cpld::CPLD hackrf_cpld { jtag_target_hackrf_cpld };

	hackrf_cpld.write_sram(hackrf::one::cpld::verify_blocks);
	const auto ok = hackrf_cpld.verify_sram(hackrf::one::cpld::verify_blocks);

	return ok;
}

bool verify_eeprom() {
	auto jtag_target_hackrf_cpld = jtag_target_hackrf();
	hackrf::one::cpld::CPLD hackrf_cpld { jtag_target_hackrf_cpld };

	const auto ok = hackrf_cpld.verify_eeprom(hackrf::one::cpld::verify_blocks);
	
	return ok;
}

void init_from_eeprom() {
	auto jtag_target_hackrf_cpld = jtag_target_hackrf();
	hackrf::one::cpld::CPLD hackrf_cpld { jtag_target_hackrf_cpld };

	hackrf_cpld.init_from_eeprom();
}

} /* namespace cpld */
} /* namespace hackrf */
