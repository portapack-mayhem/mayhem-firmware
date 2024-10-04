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
    YOU have to derive your class from I2cDev and override the init(), and the update()
    The update() query the device, and send the corresponding Message to the system with the data.
    The init() must check the device if it is really that this driver meant to handle, and fully set the device up. If all ok, set the device's model to the corresponting enum value from "i2cdevlist.hpp" and return true. Othervise false, so next driver can check it. (since multiple different devices manufactured with the same addr)

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

// Register defines
#define BMX280_REG_DIG_T1 0x88  //!< Temperature coefficient register
#define BMX280_REG_DIG_T2 0x8A  //!< Temperature coefficient register
#define BMX280_REG_DIG_T3 0x8C  //!< Temperature coefficient register

#define BMX280_REG_DIG_P1 0x8E  //!< Pressure coefficient register
#define BMX280_REG_DIG_P2 0x90  //!< Pressure coefficient register
#define BMX280_REG_DIG_P3 0x92  //!< Pressure coefficient register
#define BMX280_REG_DIG_P4 0x94  //!< Pressure coefficient register
#define BMX280_REG_DIG_P5 0x96  //!< Pressure coefficient register
#define BMX280_REG_DIG_P6 0x98  //!< Pressure coefficient register
#define BMX280_REG_DIG_P7 0x9A  //!< Pressure coefficient register
#define BMX280_REG_DIG_P8 0x9C  //!< Pressure coefficient register
#define BMX280_REG_DIG_P9 0x9E  //!< Pressure coefficient register

#define BME280_REG_DIG_H1 0xA1  //!< Humidity coefficient register
#define BME280_REG_DIG_H2 0xE1  //!< Humidity coefficient register
#define BME280_REG_DIG_H3 0xE3  //!< Humidity coefficient register
#define BME280_REG_DIG_H4 0xE4  //!< Humidity coefficient register
#define BME280_REG_DIG_H5 0xE5  //!< Humidity coefficient register
#define BME280_REG_DIG_H6 0xE7  //!< Humidity coefficient register

#define BME280_REG_CHIPID 0xD0  //!< Chip ID register
#define BME280_REG_RESET 0xE0   //!< Reset register

#define BME280_REG_CTRL_HUM 0xF2   //!< BME280: Control humidity register
#define BMX280_REG_STATUS 0XF3     //!< Status register
#define BMX280_REG_CTRL_MEAS 0xF4  //!< Control measure register
#define BMX280_REG_CONFIG 0xF5     //!< Config register
#define BMX280_REG_PRESS 0xF7      //!< Pressure data register
#define BMX280_REG_TEMP 0xFA       //!< Temperature data register
#define BME280_REG_HUM 0xFD        //!< Humidity data register

// Bit defines
#define CHIP_ID_BMP280 0x58  //!< BMP280 chip ID
#define CHIP_ID_BME280 0x60  //!< BME280 chip ID
#define RESET_KEY 0xB6       //!< Reset value for reset register
#define STATUS_IM_UPDATE 0   //!< im_update bit in status register

#define BMX280_SAMPLING_X4 0b011
#define BMX280_SAMPLING_X16 0b101
#define BMX280_FILTER_OFF 0
#define BMX280_STANDBY_MS_0_5 0
#define BMX280_MODE_NORMAL 0b11
#define BMX280_MODE_SLEEP 0b00

class I2cDev_BMX280 : public I2cDev {
   public:
    bool init(uint8_t addr_) override;  // sets the addr to our local variable, set the model, try to init the module, and only return true if it is really that module, and inited ok
    void update() override;             // query the module for recent data, and send it to the system via the corresponding Message

   private:
    // driver specific stuff
    void read_coeff();
    void set_sampling();
    float read_temperature();
    float read_pressure();
    float read_humidity();
    bool is_reading_calib();
    uint16_t _dig_T1 = 0;
    int16_t _dig_T2 = 0;
    int16_t _dig_T3 = 0;

    uint16_t _dig_P1 = 0;
    int16_t _dig_P2 = 0;
    int16_t _dig_P3 = 0;
    int16_t _dig_P4 = 0;
    int16_t _dig_P5 = 0;
    int16_t _dig_P6 = 0;
    int16_t _dig_P7 = 0;
    int16_t _dig_P8 = 0;
    int16_t _dig_P9 = 0;

    uint8_t _dig_H1 = 0;
    int16_t _dig_H2 = 0;
    uint8_t _dig_H3 = 0;
    int16_t _dig_H4 = 0;
    int16_t _dig_H5 = 0;
    int8_t _dig_H6 = 0;

    int32_t _t_fine = 0;
};
}  // namespace i2cdev

#endif