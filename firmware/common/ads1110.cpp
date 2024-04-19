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
    detected();
}

bool ADS1110::detected() {
    uint8_t status = bus.transmit(bus_address, nullptr, 0);
    return status == 0;
}

bool ADS1110::write(const address_t reg_address, const reg_t value) {
    const uint16_t word = (reg_address << 9) | value;
    const std::array<uint8_t, 2> values{
        static_cast<uint8_t>(word >> 8),
        static_cast<uint8_t>(word & 0xff),
    };
    return bus.transmit(bus_address, values.data(), values.size());
}


float ADS1110::readVoltage() {
    // Start a conversion
    write(0x00, 0x8C); // Configure the ADS1110 for single-shot conversion
    // write(0x00, 0x00); // Configure the ADS1110 for a conversion

    // Wait for the conversion to complete
    chThdSleepMilliseconds(100);

    // Read the conversion result
    uint8_t data[2];
    if (!bus.receive(bus_address, data, 2)) {
        return 0.0f; // Return 0 if the read fails
    }

    uint16_t raw = (static_cast<uint16_t>(data[0]) << 8) | data[1];

    // Convert the raw value to voltage
    // float voltage = (raw * BATTERY_MAX_VOLTAGE) / 32768.0;
    //(float)raw/ 65535.0f * 3.3f *2
    float voltage = ((float)raw / 65535.0) * 3.3 * 2; // Assuming Vdd is 3.3V
    

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
