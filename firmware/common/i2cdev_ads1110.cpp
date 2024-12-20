/*
 * Copyright (C) 2024 jLynx.
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

#include "i2cdev_ads1110.hpp"
#include "utility.hpp"
#include <algorithm>
#include <cstdint>

namespace i2cdev {

bool I2cDev_ADS1110::init(uint8_t addr_) {
    if (addr_ != I2CDEV_ADS1110_ADDR_1) return false;
    addr = addr_;
    model = I2CDEVMDL_ADS1110;
    query_interval = BATTERY_WIDGET_REFRESH_INTERVAL;

    if (detect()) {
        // Set the configuration register
        write(0x8C);
        return true;
    }
    return false;
}

bool I2cDev_ADS1110::detect() {
    uint8_t data[3];
    if (i2cbus.receive(addr, data, 3)) {
        // Check if the received data is valid
        uint8_t configRegister = data[2];
        if ((configRegister & 0x0F) == 0x0C) {
            // The configuration register value matches the expected value (0x8C)
            return true;
        }
    }
    return false;
}

void I2cDev_ADS1110::update() {
    uint16_t voltage = readVoltage();
    uint8_t batteryPercentage = battery::BatteryManagement::calc_percent_voltage(voltage);
    // send local message
    BatteryStateMessage msg{1, batteryPercentage, false, voltage};
    EventDispatcher::send_message(msg);
}

bool I2cDev_ADS1110::write(const uint8_t value) {
    return i2cbus.transmit(addr, &value, 1);
}

// returns the batt voltage in mV
uint16_t I2cDev_ADS1110::readVoltage() {
    // Read the conversion result
    uint8_t data[3];
    if (!i2cbus.receive(addr, data, 3)) {
        return 0.0f;  // Return 0 if the read fails
    }
    uint16_t raw = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    // Calculate the voltage based on the output code
    int16_t voltage = 0;
    float minCode = 0;
    float pga = 0.0f;

    uint8_t pga_rate = data[2] & 0x03;
    switch (pga_rate) {
        case 0:
            pga = 1.0f;
            break;
        case 1:
            pga = 2.0f;
            break;
        case 2:
            pga = 4.0f;
            break;
        case 3:
            pga = 8.0f;
            break;
        default:
            // Handle invalid data rate
            break;
    }

    uint8_t data_rate = (data[2] >> 2) & 0x03;
    switch (data_rate) {
        case 0:  // 240
            minCode = -2048.0;
            break;
        case 1:  // 60
            minCode = -8192.0;
            break;
        case 2:  // 30
            minCode = -16384.0;
            break;
        case 3:  // 15
            minCode = -32768.0;
            break;
        default:
            // Handle invalid data rate
            break;
    }

    // 2.048 is the reference voltage & 2.0 is to make up for the voltage divider
    voltage = (int16_t)(raw / (-1.0 * minCode) * pga * 2.048 * 2.0 * 1000.0);  // v to mV
    if (voltage < 0) voltage *= -1;                                            // should not happen in this build, but prevent it
    return (uint16_t)voltage;
}

} /* namespace i2cdev */