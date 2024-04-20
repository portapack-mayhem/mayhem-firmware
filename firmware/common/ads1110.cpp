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

#include "ads1110.hpp"
#include "utility.hpp"
#include <cstdint>
#include <algorithm>

namespace ads1110 {

constexpr float BATTERY_MIN_VOLTAGE = 3.0;
constexpr float BATTERY_MAX_VOLTAGE = 4.0;

void ADS1110::init() {
    // detected();
    // Start a conversion
    write(0x8C);
}

bool ADS1110::detected() {
    uint8_t status = bus.transmit(bus_address, nullptr, 0);
    return status == 0;
}

bool ADS1110::write(const uint8_t value) {
    return bus.transmit(bus_address, &value, 1);
}


float ADS1110::readVoltage() {
    // Read the conversion result
    uint8_t data[3];
    if (!bus.receive(bus_address, data, 3)) {
        return 0.0f; // Return 0 if the read fails
    }

    uint16_t raw = (static_cast<uint16_t>(data[0]) << 8) | data[1];

    // Calculate the voltage based on the output code
    float voltage = 0.0f;
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
        case 0: // 240
            minCode = -2048.0;
            break;
        case 1: // 60
            minCode = -8192.0;
            break;
        case 2: // 30
            minCode = -16384.0;
            break;
        case 3: // 15
            minCode = -32768.0;
            break;        
        default:
            // Handle invalid data rate
            break;
    }

    // 2.048 is the reference voltage & 2.0 is to make up for the voltage divider
    voltage = raw / (-1.0 * minCode) * pga * 2.048 * 2.0;

    return voltage;
}

void ADS1110::getBatteryInfo(float& batteryPercentage, float& voltage) {
    voltage = readVoltage();

    // Calculate the remaining battery percentage
    batteryPercentage = (voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;

    // Limit the values to the valid range
    batteryPercentage = std::clamp(batteryPercentage, 0.0f, 100.0f);
}

} /* namespace ads1110 */
