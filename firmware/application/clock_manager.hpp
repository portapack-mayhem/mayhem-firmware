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
	enum ReferenceSource {
		Xtal,     /* 10 MHz crystal onboard the HackRF */
		PortaPack, /* 10 MHz TCXO on 20180820 and newer PortaPack revisions. */
		External, /* HackRF external clock input SMA, or from PortaPack with TCXO feature. */
	};

	constexpr ClockManager(
		I2C& i2c0,
		si5351::Si5351& clock_generator
	) : i2c0(i2c0),
		clock_generator(clock_generator),
		reference_source(ReferenceSource::Xtal)/*
		_clock_f(0)*/
	{
	}

	void init_peripherals();
	void init_clock_generator();
	void shutdown();

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

	uint32_t get_frequency_monitor_measurement_in_hertz();

	ReferenceSource get_reference_source() const;

private:
	I2C& i2c0;
	si5351::Si5351& clock_generator;
	ReferenceSource reference_source;
	//uint32_t _clock_f;

	void set_gp_clkin_to_clkin_direct();

	void start_frequency_monitor_measurement(const cgu::CLK_SEL clk_sel);
	void wait_For_frequency_monitor_measurement_done();

	void enable_xtal_oscillator();
	void disable_xtal_oscillator();

	void set_m4_clock_to_irc();

	void set_m4_clock_to_pll1();
	void power_down_pll1();

	void stop_peripherals();

	uint32_t measure_gp_clkin_frequency();

	ClockManager::ReferenceSource detect_reference_source();
	ClockManager::ReferenceSource choose_reference_source();
};

#endif/*__CLOCK_MANAGER_H__*/
