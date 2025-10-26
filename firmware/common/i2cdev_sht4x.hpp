/*
 * Copyright (C) 2025 HTotoo.
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

#ifndef __I2CDEV_SHT4X_H__
#define __I2CDEV_SHT4X_H__

#include "i2cdevmanager.hpp"

namespace i2cdev {

class I2cDev_SHT4x : public I2cDev {
   public:
    typedef enum {
        SHT4X_MEASURE_HIGH_PRECISION_TICKS_CMD_ID = 0xfd,
        SHT4X_MEASURE_MEDIUM_PRECISION_TICKS_CMD_ID = 0xf6,
        SHT4X_MEASURE_LOWEST_PRECISION_TICKS_CMD_ID = 0xe0,
        SHT4X_ACTIVATE_HIGHEST_HEATER_POWER_LONG_TICKS_CMD_ID = 0x39,
        SHT4X_ACTIVATE_HIGHEST_HEATER_POWER_SHORT_TICKS_CMD_ID = 0x32,
        SHT4X_ACTIVATE_MEDIUM_HEATER_POWER_LONG_TICKS_CMD_ID = 0x2f,
        SHT4X_ACTIVATE_MEDIUM_HEATER_POWER_SHORT_TICKS_CMD_ID = 0x24,
        SHT4X_ACTIVATE_LOWEST_HEATER_POWER_LONG_TICKS_CMD_ID = 0x1e,
        SHT4X_ACTIVATE_LOWEST_HEATER_POWER_SHORT_TICKS_CMD_ID = 0x15,
        SHT4X_SERIAL_NUMBER_CMD_ID = 0x89,
        SHT4X_SOFT_RESET_CMD_ID = 0x94,
    } SHT4xCmdId;
    bool init(uint8_t addr_) override;  // sets the addr to our local variable, set the model, try to init the module, and only return true if it is really that module, and inited ok
    void update() override;             // query the module for recent data, and send it to the system via the corresponding Message

   private:
    float read_temperature();
    float read_humidity();
};
}  // namespace i2cdev

#endif