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

#ifndef __SI5351_H__
#define __SI5351_H__

#include <cstdint>
#include <array>
#include <algorithm>

#include "ch.h"
#include "hal.h"

#include "i2c_pp.hpp"

namespace si5351 {

using reg_t = uint8_t;

namespace Register {
	enum {
		DeviceStatus = 0,
		InterruptStatusSticky = 1,
		InterruptStatusMask = 2,
		OutputEnableControl = 3,
		OEBPinEnableControlMask = 9,
		PLLInputSource = 15,
		CLKControl_Base = 16,
		CLKControl0 = 16,
		CLKControl1 = 17,
		CLKControl2 = 18,
		CLKControl3 = 19,
		CLKControl4 = 20,
		CLKControl5 = 21,
		CLKControl6 = 22,
		CLKControl7 = 23,
		CLK3_0DisableState = 24,
		CLK7_4DisableState = 25,
		MultisynthNAParameters_Base = 26,
		MultisynthNBParameters_Base = 34,
		Multisynth0Parameters_Base = 42,
		Multisynth1Parameters_Base = 50,
		Multisynth2Parameters_Base = 58,
		Multisynth3Parameters_Base = 66,
		Multisynth4Parameters_Base = 74,
		Multisynth5Parameters_Base = 82,
		Multisynth6Parameters = 90,
		Multisynth7Parameters = 91,
		Clock6And7OutputDivider = 92,
		SpreadSpectrumParameters_Base = 149,
		VCXOParameters_Base = 162,
		CLKInitialPhaseOffset_Base = 165,
		PLLReset = 177,
		CrystalInternalLoadCapacitance = 183,
		FanoutEnable = 187,
	};
}

namespace DeviceStatus {
	using Type = uint8_t;

	enum {
		REVID_Mask              = (0b11 << 0),

		LOS_Mask                = (1 << 4),
		LOS_ValidClockAtCLKIN   = (0 << 4),
		LOS_LossOfSignalAtCLKIN = (1 << 4),

		LOL_A_Mask              = (1 << 5),
		LOL_A_PLLALocked        = (0 << 5),
		LOL_A_PLLAUnlocked      = (1 << 5),

		LOL_B_Mask              = (1 << 6),
		LOL_B_PLLBLocked        = (0 << 6),
		LOL_B_PLLBUnlocked      = (1 << 6),

		SYS_INIT_Mask           = (1 << 7),
		SYS_INIT_Complete       = (0 << 7),
		SYS_INIT_Initializing   = (1 << 7),
	};
}

struct ClockControl {
	enum ClockCurrentDrive {
		_2mA = 0b00,
		_4mA = 0b01,
		_6mA = 0b10,
		_8mA = 0b11,
	};

	enum ClockSource {
		Xtal = 0b00,
		CLKIN = 0b01,
		MS_Group = 0b10,
		MS_Self = 0b11,
	};

	enum ClockInvert {
		Normal = 0,
		Invert = 1,
	};

	enum MultiSynthSource {
		PLLA = 0,
		PLLB = 1,
	};

	enum MultiSynthMode {
		Fractional = 0,
		Integer = 1,
	};

	enum ClockPowerDown {
		Power_On = 0,
		Power_Off = 1,
	};

	reg_t CLK_IDRV : 2;
	reg_t CLK_SRC  : 2;
	reg_t CLK_INV  : 1;
	reg_t MS_SRC   : 1;
	reg_t MS_INT   : 1;
	reg_t CLK_PDN  : 1;

	constexpr ClockControl(
		ClockCurrentDrive clk_idrv,
		ClockSource clk_src,
		ClockInvert clk_inv,
		MultiSynthSource ms_src,
		MultiSynthMode ms_int,
		ClockPowerDown clk_pdn
	) : CLK_IDRV(clk_idrv),
		CLK_SRC(clk_src),
		CLK_INV(clk_inv),
		MS_SRC(ms_src),
		MS_INT(ms_int),
		CLK_PDN(clk_pdn)
	{
	}

	ClockControl clk_src(const ClockSource value) const {
		auto result = *this;
		result.CLK_SRC = value;
		return result;
	}
	
	ClockControl ms_src(const MultiSynthSource value) const {
		auto result = *this;
		result.MS_SRC = value;
		return result;
	}

	ClockControl clk_pdn(const ClockPowerDown value) const {
		auto result = *this;
		result.CLK_PDN = value;
		return result;
	}

	constexpr operator reg_t() {
		return *reinterpret_cast<reg_t*>(this);
	}

	static constexpr ClockControl power_off() {
		return {
			ClockCurrentDrive::_2mA,
			ClockSource::Xtal,
			ClockInvert::Normal,
			MultiSynthSource::PLLA,
			MultiSynthMode::Fractional,
			ClockPowerDown::Power_Off,
		};
	}
};

static_assert(sizeof(ClockControl) == 1, "ClockControl size is not eight bits");

using ClockControls = std::array<ClockControl, 8>;

namespace CrystalInternalLoadCapacitance {
	using Type = uint8_t;

	enum {
		XTAL_CL_Mask      = (0b11 << 6),
		XTAL_CL_6pF       = (0b01 << 6),
		XTAL_CL_8pF       = (0b10 << 6),
		XTAL_CL_10pF      = (0b11 << 6),
	};
}

namespace PLLInputSource {
	using Type = uint8_t;

	enum {
		PLLA_Source_Mask  = (1 << 2),
		PLLA_Source_XTAL  = (0 << 2),
		PLLA_Source_CLKIN = (1 << 2),

		PLLB_Source_Mask  = (1 << 3),
		PLLB_Source_XTAL  = (0 << 3),
		PLLB_Source_CLKIN = (1 << 3),

		CLKIN_Div_Mask    = (0b11 << 6),
		CLKIN_Div1        = (0b00 << 6),
		CLKIN_Div2        = (0b01 << 6),
		CLKIN_Div4        = (0b10 << 6),
		CLKIN_Div8        = (0b11 << 6),
	};
}

struct Inputs {
	const uint32_t f_xtal;
	const uint32_t f_clkin;
	const uint32_t clkin_div;

	constexpr uint32_t f_clkin_out() const {
		return f_clkin / clkin_div;
	}
};

using PLLReg = std::array<uint8_t, 9>;

struct PLL {
	const uint32_t f_in;
	const uint32_t a;
	const uint32_t b;
	const uint32_t c;

	constexpr uint32_t f_vco() const {
		return f_in * (a + (float)b / (float)c);
	}

	constexpr uint32_t p1() const {
		return 128 * a + (uint32_t)(128 * (float)b / (float)c) - 512;
	}

	constexpr uint32_t p2() const {
		return 128 * b - c * (uint32_t)(128 * (float)b / (float)c);
	}

	constexpr uint32_t p3() const {
		return c;
	}

	constexpr PLLReg reg(const uint8_t pll_n) const {
		return {
			uint8_t(26 + (pll_n * 8)),
			uint8_t((p3() >>  8) & 0xff),
			uint8_t((p3() >>  0) & 0xff),
			uint8_t((p1() >> 16) & 0x03),
			uint8_t((p1() >>  8) & 0xff),
			uint8_t((p1() >>  0) & 0xff),
			uint8_t(
				  (((p3() >> 16) & 0x0f) << 4)
				|  ((p2() >> 16) & 0x0f)
			),
			uint8_t((p2() >>  8) & 0xff),
			uint8_t((p2() >>  0) & 0xff),
		};
	}
};

using MultisynthFractionalReg = std::array<uint8_t, 9>;

struct MultisynthFractional {
	const uint32_t f_src;
	const uint32_t a;
	const uint32_t b;
	const uint32_t c;
	const uint32_t r_div;

	constexpr uint32_t p1() const {
		return 128 * a + (uint32_t)(128 * (float)b / (float)c) - 512;
	}

	constexpr uint32_t p2() const {
		return 128 * b - c * (uint32_t)(128 * (float)b / (float)c);
	}

	constexpr uint32_t p3() const {
		return c;
	}

	constexpr uint32_t f_out() const {
		return f_src / (a + (float)b / (float)c) / (1 << r_div);
	}

	constexpr MultisynthFractionalReg reg(const uint8_t multisynth_n) const {
		return {
			uint8_t(42 + (multisynth_n * 8)),
			uint8_t((p3() >> 8) & 0xFF),
			uint8_t((p3() >> 0) & 0xFF),
			uint8_t((r_div << 4) | (0 << 2) | ((p1() >> 16) & 0x3)),
			uint8_t((p1() >> 8) & 0xFF),
			uint8_t((p1() >> 0) & 0xFF),
			uint8_t((((p3() >> 16) & 0xF) << 4) | (((p2() >> 16) & 0xF) << 0)),
			uint8_t((p2() >> 8) & 0xFF),
			uint8_t((p2() >> 0) & 0xFF)
		};
	}
};

struct MultisynthInteger {
	const uint32_t f_src;
	const uint32_t a;
	const uint32_t r_div;

	constexpr uint8_t p1() const {
		return a;
	}

	constexpr uint32_t f_out() const {
		return f_src / a / (1 << r_div);
	}
};

using Multisynth6And7Reg = std::array<uint8_t, 4>;

constexpr Multisynth6And7Reg ms6_7_reg(
	const MultisynthInteger& ms6,
	const MultisynthInteger& ms7
) {
	return {
		Register::Multisynth6Parameters,
		uint8_t(ms6.p1() & 0xff),
		uint8_t(ms7.p1() & 0xff),
		uint8_t(((ms7.r_div & 7) << 4) | ((ms6.r_div & 7) << 0)),
	};
}

class Si5351 {
public:
	using regvalue_t = uint8_t;

	constexpr Si5351(I2C& bus, I2C::address_t address) :
		_clock_control({
			ClockControl::power_off(), ClockControl::power_off(),
			ClockControl::power_off(), ClockControl::power_off(),
			ClockControl::power_off(), ClockControl::power_off(),
			ClockControl::power_off(), ClockControl::power_off()
		}),
		_bus(bus),
		_address(address),
		_output_enable(0x00)
	{
	}

	void reset();

	uint8_t device_status() {
		return read_register(Register::DeviceStatus);
	}

	void wait_for_device_ready() {
		while(device_status() & 0x80);
	}

	bool clkin_loss_of_signal() {
		return (device_status() >> 4) & 1;
	}
	
	void enable_fanout() {
		write_register(Register::FanoutEnable, 0b11010000);
	}

	void reset_plls() {
		// Datasheet recommends value 0xac, though the low nibble bits are not defined in AN619.
		write_register(Register::PLLReset, 0xac);
	}

	regvalue_t read_register(const uint8_t reg);

	template<size_t N>
	void write(const std::array<uint8_t, N>& values) {
		_bus.transmit(_address, values.data(), values.size());
	}

	void write_register(const uint8_t reg, const regvalue_t value) {
		write(std::array<uint8_t, 2>{
			reg, value
		});
	}

	void write(const size_t ms_number, const MultisynthFractional& config) {
		write(config.reg(ms_number));
	}

	void set_ms_frequency(
		const size_t ms_number,
		const uint32_t frequency,
		const uint32_t vco_frequency,
		const size_t r_div
	);

	void set_crystal_internal_load_capacitance(const CrystalInternalLoadCapacitance::Type xtal_cl) {
		write_register(Register::CrystalInternalLoadCapacitance, xtal_cl);
	}

	void set_pll_input_sources(const PLLInputSource::Type value) {
		write_register(Register::PLLInputSource, value);
	}

	void enable_output_mask(const uint8_t mask) {
		_output_enable |= mask;
		update_output_enable_control();
	}

	void enable_output(const size_t n) {
		enable_output_mask(1 << n);
	}

	void disable_output_mask(const uint8_t mask) {
		_output_enable &= ~mask;
		update_output_enable_control();
	}

	void disable_output(const size_t n) {
		disable_output_mask(1 << n);
	}

	void set_clock_control(const ClockControls& clock_control) {
		_clock_control = clock_control;
		update_all_clock_control();
	}

	void set_clock_control(const size_t n, const ClockControl clock_control) {
		_clock_control[n] = clock_control;
		write_register(Register::CLKControl_Base + n, _clock_control[n]);
	}

	void enable_clock(const size_t n) {
		_clock_control[n].CLK_PDN = ClockControl::ClockPowerDown::Power_On;
		write_register(Register::CLKControl_Base + n, _clock_control[n]);
	}

	void disable_clock(const size_t n) {
		_clock_control[n].CLK_PDN = ClockControl::ClockPowerDown::Power_Off;
		write_register(Register::CLKControl_Base + n, _clock_control[n]);
	}
	
	bool clkin_status() {
		return ((device_status() & DeviceStatus::LOS_Mask) == DeviceStatus::LOS_ValidClockAtCLKIN);
	}
	
	template<size_t N>
	void write_registers(const uint8_t reg, const std::array<uint8_t, N>& values) {
		std::array<uint8_t, N + 1> data;
		data[0] = reg;
		std::copy(values.cbegin(), values.cend(), data.begin() + 1);
		write(data);
	}

private:
	ClockControls _clock_control;
	I2C& _bus;
	const I2C::address_t _address;
	uint8_t _output_enable;
	
	void update_output_enable_control() {
		write_register(Register::OutputEnableControl, ~_output_enable);
	}

	void update_all_clock_control() {
		write_registers(Register::CLKControl_Base, std::array<reg_t, 8> { {
			_clock_control[0],
			_clock_control[1],
			_clock_control[2],
			_clock_control[3],
			_clock_control[4],
			_clock_control[5],
			_clock_control[6],
			_clock_control[7],
		} });
	}
};

}

#endif/*__SI5351_H__*/
