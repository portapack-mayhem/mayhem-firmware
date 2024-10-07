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

#ifndef __I2CDEV_ADS1110_H__
#define __I2CDEV_ADS1110_H__

#include <cstdint>
#include <array>
#include <string>
#include "battery.hpp"
#include "i2cdevmanager.hpp"

namespace i2cdev {

using address_t = uint8_t;

class I2cDev_ADS1110 : public I2cDev {
   public:
    bool init(uint8_t addr_) override;
    void update() override;
    uint16_t readVoltage();

   private:
    bool write(const uint8_t value);
    bool detect();
};

} /* namespace i2cdev */
#endif /* __I2CDEV_ADS1110_H__ */