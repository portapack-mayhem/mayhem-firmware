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

#include "i2cdev_bmx280.hpp"

namespace i2cdev {

bool I2cDev_BMX280::init(uint8_t addr_) {
    addr = addr_;                     // store the addr
    model = I2C_DEVS::I2CDEV_BMP280;  // set the device model!!!!!!!!!!!!!!!!!!
    query_interval = 5;               // set update interval in sec

    uint8_t reg = BME280_REG_CHIPID;  // register
    uint8_t tmp = 0;                  // value. will save fw space, but harder to read code. so read comments
    i2c_read(&reg, 1, &tmp, 1);       // read chip id to tmp
    if (tmp != CHIP_ID_BMP280 && tmp != CHIP_ID_BME280) return false;
    if (tmp == CHIP_ID_BME280) model = I2C_DEVS::I2CDEV_BME280;

    // here we can be "sure" this is a bmp280, so init it

    // soft reset the ic
    reg = BME280_REG_RESET;
    tmp = RESET_KEY;
    i2c_write(&reg, 1, &tmp, 1);
    chThdSleepMilliseconds(150);  // wait to get awake

    read_coeff();
    set_sampling();
    return true;
}

void I2cDev_BMX280::update() {
    // todo
    float temp = read_temperature();
    float pressure = read_pressure();
    float hum = read_humidity();
    EnvironmentDataMessage msg{temp, hum, pressure};
    EventDispatcher::send_message(msg);
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

    if (model == I2C_DEVS::I2CDEV_BME280) {
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
    write8_1(BMX280_REG_CONFIG, (BMX280_STANDBY_MS_0_5 << 5) | (BMX280_FILTER_OFF << 2));
    if (model == I2C_DEVS::I2CDEV_BME280) write8_1(BME280_REG_CTRL_HUM, BMX280_SAMPLING_X16);
    write8_1(BMX280_REG_CTRL_MEAS, (BMX280_SAMPLING_X16 << 5) | (BMX280_SAMPLING_X16 << 2) | BMX280_MODE_NORMAL);
}

float I2cDev_BMX280::read_temperature() {
    int32_t var1, var2, adc_T;
    float temperature;

    // Read temperature registers
    uint8_t reg = BMX280_REG_TEMP;
    i2c_read(&reg, 1, ((uint8_t*)&adc_T), 3);
    adc_T >>= 4 + 8;
    // See datasheet 4.2.3 Compensation formulas
    var1 = ((((adc_T >> 3) - ((int32_t)_dig_T1 << 1))) * ((int32_t)_dig_T2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t)_dig_T1)) *
              ((adc_T >> 4) - ((int32_t)_dig_T1))) >>
             12) *
            ((int32_t)_dig_T3)) >>
           14;

    _t_fine = var1 + var2;
    temperature = ((_t_fine * 5) + 128) >> 8;
    return temperature / 100.0;
}

float I2cDev_BMX280::read_pressure() {
    int64_t var1;
    int64_t var2;
    int64_t p;
    int32_t adc_P;

    // Read temperature for t_fine
    // read_temperature();

    // Read pressure registers
    uint8_t reg = BMX280_REG_PRESS;
    i2c_read(&reg, 1, ((uint8_t*)&adc_P), 3);
    adc_P >>= 4 + 8;

    // See datasheet 4.2.3 Compensation formulas
    var1 = ((int64_t)_t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)_dig_P6;
    var2 = var2 + ((var1 * (int64_t)_dig_P5) << 17);
    var2 = var2 + (((int64_t)_dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)_dig_P3) >> 8) + ((var1 * (int64_t)_dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)_dig_P1) >> 33;

    if (var1 == 0) {
        return 0;  // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)_dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)_dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)_dig_P7) << 4);

    return (float)p / 256;
}

float I2cDev_BMX280::read_humidity() {
    int32_t v_x1_u32r;
    int32_t adc_H;
    float humidity;

    if (model != I2C_DEVS::I2CDEV_BME280) {
        return 0;
    }
    // Read temperature for t_fine
    // read_temperature();

    // Read humidity registers
    adc_H = read16_1(BME280_REG_HUM);
    // See datasheet 4.2.3 Compensation formulas
    v_x1_u32r = (_t_fine - ((int32_t)76800));
    v_x1_u32r = ((((adc_H << 14) - (((int32_t)_dig_H4) << 20) - (((int32_t)_dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                (((((((v_x1_u32r *
                       ((int32_t)_dig_H6)) >>
                      10) *
                     (((v_x1_u32r *
                        ((int32_t)_dig_H3)) >>
                       11) +
                      ((int32_t)32768))) >>
                    10) +
                   ((int32_t)2097152)) *
                      ((int32_t)_dig_H2) +
                  8192) >>
                 14);

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)_dig_H1)) >>
                              4));

    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    humidity = (v_x1_u32r >> 12);
    return humidity / 1024.0;
}

}  // namespace i2cdev