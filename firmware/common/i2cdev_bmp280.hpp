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

    Steps:
    - Add this new module name in the i2cdevlist.hpp's enum (to the end)
    - Create a hpp and cpp. (in the init SET THE MODULE to the prev enum value you created). If the filename is i2cdev_*.cpp it'll be added to cmake automatically.
    - Add this header to i2cdevmanager.cpp, and add an if statement and the init code to the I2CDevManager::found() function. (see examples)
    - Compile and test.
    - Create a PR.

    Notes: try to create a minimal code, to save FW space.

*/

#ifndef __I2CDEV_BMP280_H__
#define __I2CDEV_BMP280_H__

#include "i2cdevmanager.hpp"

namespace i2cdev {

class I2cDev_BMP280 : public I2cDev {
    bool init(uint8_t addr_) override;  // sets the addr to our local variable, set the model, try to init the module, and only return true if it is really that module, and inited ok
    void update() override;             // query the module for recent data, and send it to the system via the corresponding Message
};
}  // namespace i2cdev

#endif