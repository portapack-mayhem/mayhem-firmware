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

/*
    DEAR DEVS THIS IS AN EXAMPLE FILE FOR i2C COMMUNICATION
    YOU Have to derive your class from I2cDev and override at least the init(), and the update()
    The update() should query the device, and send the corresponding Message to the system with the data.
    The init() must check the device if it is really that this driver meant to handle, and fully set the device up.
    If all ok, set the device's model to the corresponting enum value from "i2cdevlist.hpp" and return true. Othervise false, so next driver can check it. (since multiple different devices manufactured with the same addr)

    You can create custom functions, that can be called from any app that identifies the device and casts the I2cDev to your class.
    This can be checked by query the 'model' variable.

*/

#include "i2cdev_bmp280.hpp"

namespace i2cdev {

bool I2cDev_BMP280::init(uint8_t addr_) {
    addr = addr_;                     // store the addr
    model = I2C_DEVS::I2CDEV_BMP280;  // set the device model!!!!!!!!!!!!!!!!!!
    query_interval = 5;               // set update interval in sec

        return false;
}

void I2cDev_BMP280::update() {
}

}  // namespace i2cdev
