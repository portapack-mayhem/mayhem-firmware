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

#ifndef __I2C_PP_H__
#define __I2C_PP_H__

#include <cstdint>

#include "ch.h"
#include "hal.h"

struct I2CClockConfig {
	float clock_source_f;
	float bus_f;
	float high_period_ns;

	static constexpr float period_ns(const float f) {
		return 1e9 / f;
	}

	constexpr uint32_t i2c_period_count() const {
		return period_ns(bus_f) / period_ns(clock_source_f) + 0.5f;
	}

	constexpr uint32_t i2c_high_count() const {
		return high_period_ns / period_ns(clock_source_f) + 0.5f;
	}

	constexpr uint32_t i2c_low_count() const {
		return i2c_period_count() - i2c_high_count();
	}
};

class I2C {
public:
	using address_t = uint8_t;

	constexpr I2C(I2CDriver* const driver) :
		_driver(driver) {
	}

	void start(const I2CConfig& config);
	void stop();

	bool receive(
		const address_t slave_address,
		uint8_t* const data, const size_t count,
		const systime_t timeout = TIME_INFINITE
	);

	bool transmit(
		const address_t slave_address,
		const uint8_t* const data, const size_t count,
		const systime_t timeout = TIME_INFINITE
	);

private:
	I2CDriver* const _driver;

	bool transfer(
		const address_t slave_address,
		const uint8_t* const data_tx, const size_t count_tx,
		uint8_t* const data_rx, const size_t count_rx,
		const systime_t timeout
	);
};

#endif/*__I2C_PP_H__*/
