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

#include "ads1100.hpp"
#include "utility.hpp"
#include <cstdint>
#include <algorithm>

namespace ads1100 {

constexpr float BATTERY_MIN_VOLTAGE = 3.0;
constexpr float BATTERY_MAX_VOLTAGE = 4.2;
constexpr float BATTERY_CAPACITY = 2500.0;
constexpr float BATTERY_ENERGY = 9.25;

void ADS1100::init() {
    detected();
}

bool ADS1100::detected() {
    uint8_t status = bus.write(bus_address, nullptr, 0);
    return status == 0;
}

bool ADS1100::write(const Register reg) {
    return write(toUType(reg), map.w[toUType(reg)]);
}

bool ADS1100::write(const address_t reg_address, const reg_t value) {
    map.w[reg_address] = value;

    const uint16_t word = (reg_address << 9) | value;
    const std::array<uint8_t, 2> values{
        static_cast<uint8_t>(word >> 8),
        static_cast<uint8_t>(word & 0xff),
    };
    return bus.transmit(bus_address, values.data(), values.size());
}

uint32_t ADS1100::reg_read(const size_t reg_address) {
    return map.w[reg_address];
}

void ADS1100::reg_write(const size_t reg_address, uint32_t value) {
    write(reg_address, value);
}

float ADS1100::readVoltage() {
    // Start a conversion
    write(0x00, 0x00);

    // Wait for the conversion to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read the conversion result
    uint16_t raw = (reg_read(0x00) << 8) | reg_read(0x01);

    // Convert the raw value to voltage
    float voltage = (raw * 4.096) / 32768.0;

    return voltage;
}

void ADS1100::getBatteryInfo(float& remainingCapacity, float& remainingEnergy, float& batteryPercentage) {
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

} /* namespace ads1100 */
