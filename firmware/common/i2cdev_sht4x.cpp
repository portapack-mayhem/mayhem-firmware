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

#include "i2cdev_sht4x.hpp"

namespace i2cdev {

bool I2cDev_SHT4x::init(uint8_t addr_) {
    if (addr_ != I2CDEV_SHT4X_ADDR_1 && addr_ != I2CDEV_SHT4X_ADDR_2) return false;
    addr = addr_;                         // store the addr
    model = I2C_DEVMDL::I2CDEVMDL_SHT4X;  // set the device model!!!!!!!!!!!!!!!!!!
    query_interval = 10;                  // set update interval in sec
    chThdSleepMilliseconds(50);
    uint8_t tmp[1];  // command buffer
    uint8_t rx[6];   // response buffer
    tmp[0] = SHT4X_SERIAL_NUMBER_CMD_ID;
    if (!i2c_write(nullptr, 0, tmp, 1)) return false;  // get serial number
    chThdSleepMilliseconds(10);
    if (!i2c_read(nullptr, 0, rx, 6)) return false;  // read serial number
    return true;
}

void I2cDev_SHT4x::update() {
    float temp = 0;
    float hum = 0;
    uint8_t cmd[1];
    cmd[0] = SHT4X_MEASURE_MEDIUM_PRECISION_TICKS_CMD_ID;
    uint8_t res[6];
    if (!i2c_write(nullptr, 0, cmd, 1)) return;
    chThdSleepMilliseconds(5);  // wait for measurement, 10 for high precision, 5 for medium, 2 for low
    if (!i2c_read(nullptr, 0, res, 6)) return;
    uint16_t temptick = (uint16_t)res[0] << 8 | (uint16_t)res[1];
    uint16_t humtick = (uint16_t)res[3] << 8 | (uint16_t)res[4];

    temp = -45 + (((float)temptick * 175.0) / 65535.0);
    hum = -6 + 125.0 * (float)humtick / 65535.0;
    if (hum < 0) hum = 0;
    if (hum > 100) hum = 100;
    EnvironmentDataMessage msg{temp, hum};  // create the system message
    EventDispatcher::send_message(msg);     // and send it
}

}  // namespace i2cdev