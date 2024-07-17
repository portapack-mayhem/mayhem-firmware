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
namespace battery {
namespace ads1110 {

using address_t = uint8_t;

class ADS1110 {
   public:
    constexpr ADS1110(I2C& bus, const I2C::address_t bus_address)
        : bus(bus), bus_address(bus_address), detected_(false) {}

    void init();
    bool detect();
    bool isDetected() const { return detected_; }

    uint16_t readVoltage();
    void getBatteryInfo(uint8_t& valid_mask, uint8_t& batteryPercentage, uint16_t& voltage);

   private:
    I2C& bus;
    const I2C::address_t bus_address;
    bool detected_;

    bool write(const uint8_t value);
};

} /* namespace ads1110 */
}  // namespace battery
#endif /* __ADS1110_H__ */