/*
 * Copyright (C) 2024 HTotoo.
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

#include "i2cdev_bh1750.hpp"
#include "utility.hpp"
#include <algorithm>
#include <cstdint>

namespace i2cdev {

#define BH1750_CONV_FACTOR 1.2
#define BH1750_MODE 0x13

bool I2cDev_BH1750::init(uint8_t addr_) {
    if (addr_ != I2CDEV_BH1750_ADDR_1) return false;
    addr = addr_;
    model = I2CDEVMDL_BH1750;
    query_interval = 3;

    // power up
    uint8_t tmp = 0x01;
    if (!i2c_write(nullptr, 0, &tmp, 1)) {
        return false;
    }
    chThdSleepMilliseconds(10);
    // set mode
    tmp = BH1750_MODE;
    if (!i2c_write(nullptr, 0, &tmp, 1)) {
        return false;
    }
    chThdSleepMilliseconds(10);

    /*
    // unused, since default
    // mtreg
    tmp = (0b01000 << 3) | (69 >> 5);
    if (!i2c_write(nullptr, 0, &tmp, 1)) {
        return false;
    }
    chThdSleepMilliseconds(10);
    tmp = (0b011 << 5) | (69 & 0b11111);
    if (!i2c_write(nullptr, 0, &tmp, 1)) {
        return false;
    }
    chThdSleepMilliseconds(10);
    // mode
    tmp = BH1750_MODE;
    if (!i2c_write(nullptr, 0, &tmp, 1)) {
        return false;
    }
    chThdSleepMilliseconds(10);
    */
    return true;
}

void I2cDev_BH1750::update() {
    uint16_t light = readLight();
    LightDataMessage msg{light};
    EventDispatcher::send_message(msg);
}

uint16_t I2cDev_BH1750::readLight() {
    float level = -1.0;
    uint8_t tmp[2];
    // Read two bytes from the sensor, which are low and high parts of the sensor value
    if (i2c_read(nullptr, 0, tmp, 2)) {
        uint16_t t = (tmp[0] << 8) | tmp[1];
        level = t;
    }

    if (level != -1.0) {
        // Convert raw value to lux
        level /= BH1750_CONV_FACTOR;
    } else
        return 0;
    return level;
}

} /* namespace i2cdev */