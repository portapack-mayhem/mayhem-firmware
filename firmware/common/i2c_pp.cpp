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

#include "i2c_pp.hpp"

void I2C::start(const I2CConfig& config) {
	i2cStart(_driver, &config);
}

void I2C::stop() {
	i2cStop(_driver);
}

bool I2C::transfer(
	const address_t slave_address,
	const uint8_t* const data_tx, const size_t count_tx,
	uint8_t* const data_rx, const size_t count_rx,
	systime_t timeout
) {
	i2cAcquireBus(_driver);
	const msg_t status = i2cMasterTransmitTimeout(
		_driver, slave_address, data_tx, count_tx, data_rx, count_rx, timeout
	);
	i2cReleaseBus(_driver);
	return (status == RDY_OK);
}

bool I2C::receive(
	const address_t slave_address,
	uint8_t* const data, const size_t count,
	systime_t timeout
) {
	i2cAcquireBus(_driver);
	const msg_t status = i2cMasterReceiveTimeout(
		_driver, slave_address, data, count, timeout
	);
	i2cReleaseBus(_driver);
	return (status == RDY_OK);
}

bool I2C::transmit(
	const address_t slave_address,
	const uint8_t* const data, const size_t count,
	systime_t timeout
) {
	return transfer(slave_address, data, count, NULL, 0, timeout);
}
