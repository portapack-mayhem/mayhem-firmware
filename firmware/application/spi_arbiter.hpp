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

#ifndef __SPI_ARBITER_H__
#define __SPI_ARBITER_H__

#include <cstddef>

#include "spi_pp.hpp"

namespace spi {
namespace arbiter {

class Arbiter {
public:
	constexpr Arbiter(
		SPI& bus
	) : _bus(bus),
		_config(nullptr)
	{
	}

	void transfer(const SPIConfig* const config, void* const data, const size_t count) {
		if( config != _config ) {
			_bus.stop();
			_bus.start(*config);
			_config = config;
		}
		_bus.transfer(data, count);
	}

private:
	SPI& _bus;
	const SPIConfig* _config;
};

class Target {
public:
	constexpr Target(
		Arbiter& arbiter,
		const SPIConfig& config
	) : _arbiter(arbiter),
		_config(config)
	{
	}

	void transfer(void* const data, const size_t count) {
		_arbiter.transfer(&_config, data, count);
	}

private:
	Arbiter& _arbiter;
	const SPIConfig _config;
};

} /* arbiter */
} /* spi */

#endif/*__SPI_ARBITER_H__*/
