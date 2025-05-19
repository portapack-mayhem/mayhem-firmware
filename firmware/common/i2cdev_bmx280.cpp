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

#include "i2cdev_bmx280.hpp"

namespace i2cdev {

bool I2cDev_BMX280::init(uint8_t addr_) {
    if (addr_ != I2CDEV_BMX280_ADDR_1 && addr_ != I2CDEV_BMX280_ADDR_2) return false;
    addr = addr_;                          // store the addr so i2c write / read will use this.
    model = I2C_DEVMDL::I2CDEVMDL_BMP280;  // set the device model!!!!!!!!!!!!!!!!!!
    query_interval = 5;                    // set update interval in sec

    uint8_t reg = BME280_REG_CHIPID;                                   // register
    uint8_t tmp = 0;                                                   // value. will save fw space, but harder to read code. so read comments
    i2c_read(&reg, 1, &tmp, 1);                                        // read chip id to tmp
    if (tmp != CHIP_ID_BMP280 && tmp != CHIP_ID_BME280) return false;  // this is not BME280 or BMP280, so skip
    if (tmp == CHIP_ID_BME280) model = I2C_DEVMDL::I2CDEVMDL_BME280;   // update dev model, since this driver can handle 2 type of models

    // here we can be "sure" this is a bmp280, so init it

    // soft reset the ic
    reg = BME280_REG_RESET;
    tmp = RESET_KEY;
    i2c_write(&reg, 1, &tmp, 1);
    chThdSleepMilliseconds(10);  // wait to get awake
    uint8_t timeout = 0;         // wait for calibration data load
    while (is_reading_calib()) {
        timeout++;
        if (timeout > 200) return false;  // timeout, bad device
        chThdSleepMilliseconds(10);
    }
    read_coeff();
    set_sampling();
    chThdSleepMilliseconds(50);
    return true;
}

void I2cDev_BMX280::update() {
    float temp = read_temperature();  // internal data gathering from the device.
    float pressure = read_pressure();
    float hum = read_humidity();
    EnvironmentDataMessage msg{temp, hum, pressure};  // create the system message
    EventDispatcher::send_message(msg);               // and send it
}

/*

    INTERNAL FUNCTIONS

*/

bool I2cDev_BMX280::is_reading_calib() {
    uint8_t const rStatus = read8_1(BMX280_REG_STATUS);
    return (rStatus & (1 << 0)) != 0;
}

void I2cDev_BMX280::read_coeff() {
    _dig_T1 = read16_LE_1(BMX280_REG_DIG_T1);
    _dig_T2 = readS16_LE_1(BMX280_REG_DIG_T2);
    _dig_T3 = readS16_LE_1(BMX280_REG_DIG_T3);

    _dig_P1 = read16_LE_1(BMX280_REG_DIG_P1);
    _dig_P2 = readS16_LE_1(BMX280_REG_DIG_P2);
    _dig_P3 = readS16_LE_1(BMX280_REG_DIG_P3);
    _dig_P4 = readS16_LE_1(BMX280_REG_DIG_P4);
    _dig_P5 = readS16_LE_1(BMX280_REG_DIG_P5);
    _dig_P6 = readS16_LE_1(BMX280_REG_DIG_P6);
    _dig_P7 = readS16_LE_1(BMX280_REG_DIG_P7);
    _dig_P8 = readS16_LE_1(BMX280_REG_DIG_P8);
    _dig_P9 = readS16_LE_1(BMX280_REG_DIG_P9);

    if (model == I2C_DEVMDL::I2CDEVMDL_BME280) {
        _dig_H1 = read8_1(BME280_REG_DIG_H1);
        _dig_H2 = readS16_LE_1(BME280_REG_DIG_H2);
        _dig_H3 = read8_1(BME280_REG_DIG_H3);
        _dig_H4 = ((int8_t)read8_1(BME280_REG_DIG_H4) << 4) | (read8_1(BME280_REG_DIG_H4 + 1) & 0xF);
        _dig_H5 = ((int8_t)read8_1(BME280_REG_DIG_H5 + 1) << 4) | (read8_1(BME280_REG_DIG_H5) >> 4);
        _dig_H6 = (int8_t)read8_1(BME280_REG_DIG_H6);
    }
}

void I2cDev_BMX280::set_sampling() {
    //
    write8_1(BMX280_REG_CTRL_MEAS, BMX280_MODE_SLEEP);
    write8_1(BMX280_REG_CONFIG, (uint8_t)((BMX280_STANDBY_MS_0_5 << 5) | (BMX280_FILTER_OFF << 2)));
    if (model == I2C_DEVMDL::I2CDEVMDL_BME280) write8_1(BME280_REG_CTRL_HUM, BMX280_SAMPLING_X16);
    write8_1(BMX280_REG_CTRL_MEAS, (uint8_t)((BMX280_SAMPLING_X16 << 5) | (BMX280_SAMPLING_X16 << 2) | BMX280_MODE_NORMAL));
}

float I2cDev_BMX280::read_temperature() {
    int32_t var1, var2;

    int32_t adc_T = read24_1(BMX280_REG_TEMP);
    if (adc_T == 0x800000)  // value in case temp measurement was disabled
        return 0;
    adc_T >>= 4;

    var1 = (int32_t)((adc_T / 8) - ((int32_t)_dig_T1 * 2));
    var1 = (var1 * ((int32_t)_dig_T2)) / 2048;
    var2 = (int32_t)((adc_T / 16) - ((int32_t)_dig_T1));
    var2 = (((var2 * var2) / 4096) * ((int32_t)_dig_T3)) / 16384;

    _t_fine = var1 + var2;  // + t_fine_adjust;

    int32_t T = (_t_fine * 5 + 128) / 256;
    return (float)T / 100;
}

float I2cDev_BMX280::read_pressure() {
    int64_t var1, var2, var3, var4;

    // readTemperature(); // must be done first to get t_fine

    int32_t adc_P = read24_1(BMX280_REG_PRESS);
    if (adc_P == 0x800000)  // value in case pressure measurement was disabled
        return 0;
    adc_P >>= 4;

    var1 = ((int64_t)_t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)_dig_P6;
    var2 = var2 + ((var1 * (int64_t)_dig_P5) * 131072);
    var2 = var2 + (((int64_t)_dig_P4) * 34359738368);
    var1 = ((var1 * var1 * (int64_t)_dig_P3) / 256) +
           ((var1 * ((int64_t)_dig_P2) * 4096));
    var3 = ((int64_t)1) * 140737488355328;
    var1 = (var3 + var1) * ((int64_t)_dig_P1) / 8589934592;

    if (var1 == 0) {
        return 0;  // avoid exception caused by division by zero
    }

    var4 = 1048576 - adc_P;
    var4 = (((var4 * 2147483648) - var2) * 3125) / var1;
    var1 = (((int64_t)_dig_P9) * (var4 / 8192) * (var4 / 8192)) /
           33554432;
    var2 = (((int64_t)_dig_P8) * var4) / 524288;
    var4 = ((var4 + var1 + var2) / 256) + (((int64_t)_dig_P7) * 16);

    float P = var4 / 256.0;
    return P / 100;
}

float I2cDev_BMX280::read_humidity() {
    if (model != I2C_DEVMDL::I2CDEVMDL_BME280) return 0;
    int32_t var1, var2, var3, var4, var5;
    // readTemperature();  // must be done first to get t_fine
    int32_t adc_H = read16_1(BME280_REG_HUM);
    if (adc_H == 0x8000)  // value in case humidity measurement was disabled
        return 0;

    var1 = _t_fine - ((int32_t)76800);
    var2 = (int32_t)(adc_H * 16384);
    var3 = (int32_t)(((int32_t)_dig_H4) * 1048576);
    var4 = ((int32_t)_dig_H5) * var1;
    var5 = (((var2 - var3) - var4) + (int32_t)16384) / 32768;
    var2 = (var1 * ((int32_t)_dig_H6)) / 1024;
    var3 = (var1 * ((int32_t)_dig_H3)) / 2048;
    var4 = ((var2 * (var3 + (int32_t)32768)) / 1024) + (int32_t)2097152;
    var2 = ((var4 * ((int32_t)_dig_H2)) + 8192) / 16384;
    var3 = var5 * var2;
    var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
    var5 = var3 - ((var4 * ((int32_t)_dig_H1)) / 16);
    var5 = (var5 < 0 ? 0 : var5);
    var5 = (var5 > 419430400 ? 419430400 : var5);
    uint32_t H = (uint32_t)(var5 / 4096);
    return (float)H / 1024.0;
}

}  // namespace i2cdev