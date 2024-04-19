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
constexpr float BATTERY_MAX_VOLTAGE = 4.2;
constexpr float BATTERY_CAPACITY = 2500.0;
constexpr float BATTERY_ENERGY = 9.25;

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

    // Convert the raw value to voltage
    // float voltage = ((float)raw / 65535.0) * 3.3 * 2; // Assuming Vdd is 3.3V
    // float voltage = ((float)raw / 2048.0) * 3.3 * 2; // Assuming Vdd is 3.3V
    // float voltage = 32768 * 1 * ((float)raw / 2.048); // Assuming Vdd is 3.3V
    // float voltage = (float) raw / 2048.0f * 2.048f *2.0f;

     // Calculate the voltage based on the output code
    float voltage = 0.0f;
    int16_t minCode = 0;
    float pga = 0.0f; // Assuming PGA = 1, adjust according to your configuration

    uint8_t pga_rate = data[2] & 0x03; // Assuming data rate is 240 SPS, adjust according to your configuration

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

    uint8_t data_rate = (data[2] >> 2) & 0x03; // Assuming data rate is 240 SPS, adjust according to your configuration

    switch (data_rate) {
        case 0: // 15
            minCode = -32768;
            break;
        case 1: // 30
            minCode = -16384;
            break;
        case 2: // 60
            minCode = -8192;
            break;
        case 3: // 240
            minCode = -2048;
            break;
        default:
            // Handle invalid data rate
            break;
    }

    // voltage = raw;
    // voltage = data[2];
    // voltage = (raw - (-1 * minCode)) * 2.048f / (pga * static_cast<float>(minCode * -2));
    voltage = (raw / 2.048f) * pga * (-1 * minCode);
    // voltage =  (float)raw / (float)minCode * 2.048f * 2.0f;

    // voltage = data[2];
    

    return voltage;
}

void ADS1110::getBatteryInfo(float& remainingCapacity, float& remainingEnergy, float& batteryPercentage) {
    float voltage = readVoltage();

    // Calculate the remaining capacity, energy, and percentage
    remainingCapacity = (voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * BATTERY_CAPACITY;
    remainingEnergy = (voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * BATTERY_ENERGY;
    batteryPercentage = (voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;

    // Limit the values to the valid range
    remainingCapacity = std::clamp(remainingCapacity, 0.0f, BATTERY_CAPACITY);
    remainingEnergy = std::clamp(remainingEnergy, 0.0f, BATTERY_ENERGY);
    batteryPercentage = std::clamp(batteryPercentage, 0.0f, 100.0f);
}

} /* namespace ads1110 */
