/*
 * Copyright (C) 2024 jLynx.
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

#ifndef __ADS1110_H__
#define __ADS1110_H__

#include <cstdint>
#include <array>
#include <string>

#include "i2c_pp.hpp"

namespace ads1110 {

using address_t = uint8_t;
using reg_t = uint16_t;

class ADS1110 {
public:
    constexpr ADS1110(I2C& bus, const I2C::address_t bus_address)
    : bus(bus), bus_address(bus_address) {}

    void init();
    bool detected();

    float readVoltage();
    void getBatteryInfo(float& remainingCapacity, float& remainingEnergy, float& batteryPercentage);

private:
    I2C& bus;
    const I2C::address_t bus_address;

    bool write(const uint8_t value);
};

} /* namespace ads1110 */

#endif /* __ADS1110_H__ */