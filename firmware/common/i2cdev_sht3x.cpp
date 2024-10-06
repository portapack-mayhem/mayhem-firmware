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

#include "i2cdev_sht3x.hpp"

namespace i2cdev {

bool I2cDev_SHT3x::init(uint8_t addr_) {
    if (addr_ != I2CDEV_SHT3X_ADDR_1 && addr_ != I2CDEV_SHT3X_ADDR_2) return false;
    addr = addr_;                         // store the addr
    model = I2C_DEVMDL::I2CDEVMDL_SHT3X;  // set the device model!!!!!!!!!!!!!!!!!!
    query_interval = 5;                   // set update interval in sec
    chThdSleepMilliseconds(50);
    uint8_t tmp[2];  // command buffer
    tmp[0] = 0x30;
    tmp[1] = 0x66;
    if (!i2c_write(nullptr, 0, tmp, 2)) return false;  // heater off
    chThdSleepMilliseconds(50);
    tmp[0] = 0x22;
    tmp[1] = 0x36;
    if (!i2c_write(nullptr, 0, tmp, 2)) return false;  // conti, 2mps

    chThdSleepMilliseconds(50);
    return true;
}

void I2cDev_SHT3x::update() {
    float temp = 0;
    float hum = 0;
    uint8_t cmd[2];
    cmd[0] = 0xe0;
    cmd[1] = 0;
    uint8_t res[6];
    i2c_read(cmd, 2, res, 6);
    uint16_t temptick = res[0] << 8 | res[1];
    uint16_t humtick = res[3] << 8 | res[4];

    temp = -45 + (((float)temptick * 175.0) / 65535.0);
    hum = (100.0 * (float)humtick) / 65535.0;
    EnvironmentDataMessage msg{temp, hum};  // create the system message
    EventDispatcher::send_message(msg);     // and send it
}

}  // namespace i2cdev