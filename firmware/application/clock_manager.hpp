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

#ifndef __CLOCK_MANAGER_H__
#define __CLOCK_MANAGER_H__

#include "ch.h"
#include "hal.h"

#include "i2c_pp.hpp"
#include "si5351.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

class ClockManager {
public:
	constexpr ClockManager(
		I2C& i2c0,
		si5351::Si5351& clock_generator
	) : i2c0(i2c0),
		clock_generator(clock_generator)/*,
		_clock_f(0)*/
	{
	}

	void init(const bool use_clkin);
	void shutdown();

	void run_from_irc();
	void run_at_full_speed();

	void start_audio_pll();
	void stop_audio_pll();

	void set_base_audio_clock_divider(const size_t divisor);

	void enable_codec_clocks();
	void disable_codec_clocks();

	void enable_first_if_clock();
	void disable_first_if_clock();

	void enable_second_if_clock();
	void disable_second_if_clock();

	void set_sampling_frequency(const uint32_t frequency);

	void set_reference_ppb(const int32_t ppb);

private:
	I2C& i2c0;
	si5351::Si5351& clock_generator;
	//uint32_t _clock_f;

	void change_clock_configuration(const cgu::CLK_SEL clk_sel);

	void enable_gp_clkin_source();
	void disable_gp_clkin_source();

	void enable_xtal_oscillator();
	void disable_xtal_oscillator();

	void set_m4_clock_to_irc();

	void set_m4_clock_to_pll1();
	void power_down_pll1();

	void stop_peripherals();
	void update_peripheral_clocks(const cgu::CLK_SEL clk_sel);
	void start_peripherals(const cgu::CLK_SEL clk_sel);
};

#endif/*__CLOCK_MANAGER_H__*/
