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

#include "max17055.hpp"
#include "utility.hpp"
#include <algorithm>
#include <cstdint>

namespace battery {
namespace max17055 {

constexpr uint16_t BATTERY_MIN_VOLTAGE = 3000;
constexpr uint16_t BATTERY_MAX_VOLTAGE = 4200; // Adjusted for typical Li-ion battery

void MAX17055::init() {
    if (!detected_) {
        detected_ = detect();
    }
}

bool MAX17055::detect() {
    uint16_t value;
    if (readRegister(0x00, value)) { // Read the Status register
        detected_ = true;
        return true;
    }
    detected_ = false;
    return false;
}

bool MAX17055::readRegister(uint8_t reg, uint16_t& value) {
    uint8_t data[2];
    if (bus.transmit(bus_address, &reg, 1) && bus.receive(bus_address, data, 2)) {
        value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
        return true;
    }
    return false;
}

// returns the battery voltage in mV
uint16_t MAX17055::readVoltage() {
    uint16_t rawVoltage;
    if (!readRegister(0x19, rawVoltage)) { // Read the Voltage register
        return 0;  // Return 0 if the read fails
    }

    // Voltage register value is in units of 78.125µV
    return (rawVoltage * 78125) / 1000000;
}
// uint16_t MAX17055::readVoltage() {
//     uint16_t rawVoltage;
//     if (!readRegister(0x09, rawVoltage)) { // Read the Voltage register
//         return 0;  // Return 0 if the read fails
//     }

//     // Voltage register value is in units of 1.25mV
//     return (rawVoltage * 125) / 100;
// }

uint8_t MAX17055::readPercentage() {
    uint16_t rawPercentage;
    if (!readRegister(0x06, rawPercentage)) { // Read the SOC register
        return 0;  // Return 0 if the read fails
    }

    // SOC register value is in units of 1/256 percentage
    return (rawPercentage + 128) / 256;
}

void MAX17055::getBatteryInfo(uint8_t& batteryPercentage, uint16_t& voltage) {
    voltage = readVoltage();
    batteryPercentage = readPercentage();

    // // Calculate the remaining battery percentage
    // batteryPercentage = (float)(voltage - BATTERY_MIN_VOLTAGE) / (float)(BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;

    // // Limit the values to the valid range
    // batteryPercentage = (batteryPercentage > 100) ? 100 : batteryPercentage;
    // batteryPercentage = (batteryPercentage < 0) ? 0 : batteryPercentage;
}

} /* namespace max17055 */
}  // namespace battery