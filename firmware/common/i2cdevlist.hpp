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

#ifndef __I2CDEVLIST_H__
#define __I2CDEVLIST_H__

/*
    DEAR DEVS: Put your new driver to this enum's end. Also consider using define to store it's address.
    If the same driver can handle multiple devices, with different data, should use different names. Like BMP280 + BME280. If the data is the same across multiple devices, can use the same name. Like SHT3x (sht30, sht31, ..)
*/

enum I2C_DEVMDL {
    I2CDEVMDL_NOTSET,  // i2c dev present, but no driver for it
    I2CDEVMDL_MAX17055,
    I2CDEVMDL_ADS1110,
    I2CDEVMDL_SHT3X,
    I2CDEVMDL_BMP280,
    I2CDEVMDL_BME280,
    I2CDEVMDL_BH1750,
    I2CDECMDL_PPMOD,
    I2CDEVMDL_SHT4X,
};

#define I2CDEV_BMX280_ADDR_1 0x76
#define I2CDEV_BMX280_ADDR_2 0x77
#define I2CDEV_SHT3X_ADDR_1 0x44
#define I2CDEV_SHT3X_ADDR_2 0x45
// sadly sht4x uses the same addr as sht3x, but the init script can decide which is which
#define I2CDEV_SHT4X_ADDR_1 0x44
#define I2CDEV_SHT4X_ADDR_2 0x45

#define I2CDEV_MAX17055_ADDR_1 0x36
#define I2CDEV_ADS1110_ADDR_1 0x48

#define I2CDEV_BH1750_ADDR_1 0x23

#define I2CDEV_PPMOD_ADDR_1 0x51

// this will be the update interval for battery management ic's:
#define BATTERY_WIDGET_REFRESH_INTERVAL 10

#endif