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

#include "si5351.hpp"

#include "utility.hpp"

#include <cstdint>
#include <array>

namespace si5351 {

void Si5351::reset() {
	wait_for_device_ready();

	write_register(Register::InterruptStatusSticky, 0x00);
	write_register(Register::InterruptStatusMask, 0xf0);

	update_output_enable_control();
	write_register(Register::OEBPinEnableControlMask, 0xff);
	write_register(Register::PLLInputSource, 0x00);

	_clock_control = {
		ClockControl::power_off(), ClockControl::power_off(),
		ClockControl::power_off(), ClockControl::power_off(),
		ClockControl::power_off(), ClockControl::power_off(),
		ClockControl::power_off(), ClockControl::power_off()
	};
	update_all_clock_control();

	write(std::array<uint8_t, 70> { Register::CLK3_0DisableState });

	write(std::array<uint8_t, 14>{
		Register::SpreadSpectrumParameters_Base
	});

	write(std::array<uint8_t, 4>{
		Register::VCXOParameters_Base
	});

	write(std::array<uint8_t, 7>{
		Register::CLKInitialPhaseOffset_Base
	});

	write_register(Register::CrystalInternalLoadCapacitance, 0b11010010);
	write_register(Register::FanoutEnable, 0x00);

	reset_plls();
}

Si5351::regvalue_t Si5351::read_register(const uint8_t reg) {
	const std::array<uint8_t, 1> tx { reg };
	std::array<uint8_t, 1> rx { 0x00 };
	_bus.transmit(_address, tx.data(), tx.size());
	_bus.receive(_address, rx.data(), rx.size());
	return rx[0];
}

void Si5351::set_ms_frequency(
	const size_t ms_number,
	const uint32_t frequency,
	const uint32_t vco_frequency,
	const size_t r_div
) {
	/* TODO: Factor out the VCO frequency, which should be an attribute held
	 * by the Si5351 object.
	 */
	const uint32_t a = vco_frequency / frequency;
	const uint32_t remainder = vco_frequency - (frequency * a);
	const uint32_t denom = gcd(remainder, frequency);
	const uint32_t b = remainder / denom;
	const uint32_t c = frequency / denom;

	/* TODO: Switch between integer and fractional modes depending on the
	 * values of a and b.
	 */
	const MultisynthFractional ms {
		.f_src = vco_frequency,
		.a = a,
		.b = b,
		.c = c,
		.r_div = r_div,
	};
	const auto regs = ms.reg(ms_number);
	write(regs);
}

} /* namespace si5351 */
