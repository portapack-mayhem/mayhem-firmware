#include "max17055.hpp"
#include "utility.hpp"
#include <algorithm>
#include <cstdint>

namespace battery {
namespace max17055 {

constexpr uint16_t BATTERY_MIN_VOLTAGE = 3000;
constexpr uint16_t BATTERY_MAX_VOLTAGE = 4200;

void MAX17055::init() {
    if (!detected_) {
        detected_ = detect();
    }
}

bool MAX17055::detect() {
    uint16_t deviceID = read(0x08);
    if (deviceID == 0x0055) {
        detected_ = true;
        return true;
    }
    detected_ = false;
    return false;

// detected_ = true;
// return true;
}

// bool MAX17055::detect() {
//     uint8_t regAddress = 0x08;
//     uint8_t data[2];

//     // Send the register address to the MAX17055
//     if (!bus.transmit(bus_address, &regAddress, 1)) {
//         // I2C transmission failed
//         detected_ = false;
//         return false;
//     }

//     // Read the device ID from the MAX17055
//     if (!bus.receive(bus_address, data, 2)) {
//         // I2C reception failed
//         detected_ = false;
//         return false;
//     }

//     uint16_t deviceID = (static_cast<uint16_t>(data[0]) << 8) | data[1];

//     if (deviceID == 0x0055) {
//         detected_ = true;
//         return true;
//     }

//     // Device ID does not match the expected value
//     detected_ = false;
//     return false;
// }

bool MAX17055::write(const uint8_t reg, const uint8_t value) {
    uint8_t data[2] = {reg, value};
    return bus.transmit(bus_address, data, 2);
}

uint16_t MAX17055::read(const uint8_t reg) {
    uint8_t data[2];
    if (!bus.transmit(bus_address, &reg, 1) || !bus.receive(bus_address, data, 2)) {
        return 0;  // Return 0 if the read fails
    }
    return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

uint16_t MAX17055::readVoltage() {
    uint16_t voltageRaw = read(0x09);
    float voltage = voltageRaw * 0.00125;          // Convert to voltage (assuming 5.12V full-scale range)
    return static_cast<uint16_t>(voltage * 1000);  // Convert to mV
}

void MAX17055::getBatteryInfo(uint8_t& batteryPercentage, uint16_t& voltage) {
    voltage = readVoltage();

    // Calculate the remaining battery percentage
    batteryPercentage = (float)(voltage - BATTERY_MIN_VOLTAGE) / (float)(BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;

    // Limit the values to the valid range
    batteryPercentage = (batteryPercentage > 100) ? 100 : batteryPercentage;
}

} /* namespace max17055 */
}  // namespace battery