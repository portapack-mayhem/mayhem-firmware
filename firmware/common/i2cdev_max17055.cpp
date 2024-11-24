/*

Copyright (C) 2024 jLynx.
*
This file is part of PortaPack.
*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.
*
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*
You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street,
Boston, MA 02110-1301, USA.
*/
#include "i2cdev_max17055.hpp"
#include "battery.hpp"
#include "utility.hpp"
#include "portapack_persistent_memory.hpp"
#include <cstring>
#include <algorithm>
#include <cstdint>

namespace i2cdev {

const I2cDev_MAX17055::RegisterEntry I2cDev_MAX17055::entries[] = {
    {"Status", 0x00, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"VAlrtTh", 0x01, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"TAlrtTh", 0x02, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"SAlrtTh", 0x03, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"AtRate", 0x04, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
    {"RepCap", 0x05, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"RepSOC", 0x06, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
    {"Age", 0x07, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
    {"Temp", 0x08, "temperature", 0.00390625, true, "C", true, 6, true, false, false, 0, false},
    {"VCell", 0x09, "voltage", 0.000078125, false, "V", true, 9, true, false, false, 0, false},
    {"Current", 0x0A, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
    {"AvgCurrent", 0x0B, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
    {"QResidual", 0x0C, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"MixSOC", 0x0D, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
    {"AvSOC", 0x0E, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
    {"MixCap", 0x0F, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},

    {"FullCapRep", 0x10, "capacity", 0.5, false, "mAh", true, 1, true, true, false, 0, false},
    {"TTE", 0x11, "time", 0.0015625, false, "hr", true, 6, true, false, false, 0, false},
    {"QRTable00", 0x12, "model", 1, false, "", false, 0, true, true, false, 0, false},
    {"FullSocThr", 0x13, "model", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
    {"RCell", 0x14, "resistance", 0.244140625, false, "mOhms", false, 6, true, false, false, 0, false},
    {"Reserved", 0x15, "", 0.244140625, false, "mOhms", false, 6, true, false, false, 0, true},
    {"AvgTA", 0x16, "temperature", 0.00390625, true, "C", true, 6, true, false, false, 0, false},
    {"Cycles", 0x17, "cycles", 0.01, false, "Cycles", false, 2, true, true, false, 0, false},
    {"DesignCap", 0x18, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"AvgVCell", 0x19, "voltage", 0.000078125, false, "V", true, 9, true, false, false, 0, false},
    {"MaxMinTemp", 0x1A, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"MaxMinVolt", 0x1B, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"MaxMinCurr", 0x1C, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Config", 0x1D, "model", 1, false, "", false, 0, true, false, false, 0, false},
    {"IChgTerm", 0x1E, "model", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
    {"AvCap", 0x1F, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},

    {"TTF", 0x20, "time", 0.0015625, false, "hr", true, 6, true, false, false, 0, false},
    {"DevName", 0x21, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"QRTable10", 0x22, "model", 1, false, "", false, 0, true, true, false, 0, false},
    {"FullCapNom", 0x23, "capacity", 0.5, false, "mAh", true, 1, true, true, false, 0, false},
    {"Reserved", 0x24, "", 0.00390625, true, "C", true, 6, true, false, false, 0, true},
    {"Reserved", 0x25, "", 0.00390625, true, "C", true, 6, true, false, false, 0, true},
    {"Reserved", 0x26, "", 0.00390625, true, "C", true, 6, true, false, false, 0, true},
    {"AIN", 0x27, "temperature", 0.0015259, false, "%", true, 6, true, false, false, 0, false},
    {"LearnCfg", 0x28, "model", 1, false, "", false, 0, true, false, false, 0, false},
    {"FilterCfg", 0x29, "model", 1, false, "", false, 0, true, false, false, 0, false},
    {"RelaxCfg", 0x2A, "model", 1, false, "", false, 0, true, false, false, 0, false},
    {"MiscCfg", 0x2B, "model", 1, false, "", false, 0, true, false, false, 0, false},
    {"TGain", 0x2C, "temperature", 1, false, "", false, 0, true, false, false, 0, false},
    {"TOff", 0x2D, "temperature", 1, false, "", false, 0, true, false, false, 0, false},
    {"CGain", 0x2E, "current", 0.09765625, true, "%", true, 8, true, false, false, 0, false},
    {"COff", 0x2F, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},

    {"Reserved", 0x30, "", 0.00125, false, "V", true, 5, true, false, false, 0, true},
    {"Reserved", 0x31, "", 0.005, true, "mA", true, 3, true, false, false, 0, true},
    {"QRTable20", 0x32, "model", 1, false, "", false, 0, true, true, false, 0, false},
    {"Reserved", 0x33, "", 0.0015625, false, "hr", true, 6, true, true, false, 0, true},
    {"DieTemp", 0x34, "temperature", 0.00390625, true, "C", true, 6, true, false, false, 0, false},
    {"FullCap", 0x35, "model", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"Reserved", 0x36, "", 0.15625, true, "mA", true, 5, true, false, false, 0, true},
    {"Reserved", 0x37, "led", 1, false, "", false, 0, true, true, false, 0, false},
    {"RComp0", 0x38, "model", 1, false, "", false, 0, true, true, false, 0, false},
    {"TempCo", 0x39, "model", 1, false, "", false, 0, true, true, false, 0, false},
    {"VEmpty", 0x3A, "model", 0.000078125, false, "", false, 1, true, false, false, 0, false},
    {"Reserved", 0x3B, "", 0.15625, true, "mA", true, 5, true, false, false, 0, true},
    {"Reserved", 0x3C, "", 0.000976563, false, "s", true, 6, true, false, false, 0, true},
    {"FStat", 0x3D, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Timer", 0x3E, "time", 4.88E-05, false, "hr", true, 7, true, false, false, 0, false},
    {"ShdnTimer", 0x3F, "", 0.000366, false, "hr", true, 6, true, false, false, 0, false},

    {"UserMem1", 0x40, "led", 1, false, "", false, 0, true, false, false, 0, false},
    {"Reserved", 0x41, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"QRTable30", 0x42, "model", 0.15625, false, "", false, 0, true, true, false, 0, false},
    {"RGain", 0x43, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Reserved", 0x44, "", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
    {"dQAcc", 0x45, "capacity", 16, false, "mAh", true, 0, true, true, false, 0, false},
    {"dPAcc", 0x46, "percent", 0.0625, false, "%", true, 4, true, true, false, 0, false},
    {"Reserved", 0x47, "", 0.00390625, true, "%", true, 6, true, true, false, 0, true},
    {"Reserved", 0x48, "", 0.00390625, false, "%", true, 6, true, true, false, 0, true},
    {"ConvgCfg", 0x49, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"VFRemCap", 0x4A, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"Reserved", 0x4B, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Reserved", 0x4C, "", 0.5, true, "mAh", true, 1, true, false, false, 0, true},
    {"QH", 0x4D, "capacity", 0.5, true, "mAh", true, 1, true, false, false, 0, false},
    {"Reserved", 0x4E, "", 7.63E-06, false, "mAh", true, 8, true, false, false, 0, true},
    {"Reserved", 0x4F, "", 0.5, false, "mAh", true, 1, true, false, false, 0, true},

    {"Status2", 0xB0, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Power", 0xB1, "", 0.8, true, "", false, 1, true, false, false, 0, false},
    {"ID", 0xB2, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"AvgPower", 0xB3, "", 0.8, true, "", false, 1, true, false, false, 0, false},
    {"IAlrtTh", 0xB4, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"TTFCfg", 0xB5, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"CVMixCap", 0xB6, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"CVHalfTime", 0xB7, "time", 0.000195313, false, "hr", true, 6, true, false, false, 0, false},
    {"CGTempCo", 0xB8, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Curve", 0xB9, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"HibCfg", 0xBA, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Config2", 0xBB, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"VRipple", 0xBC, "voltage", 9.77E-06, false, "V", true, 8, true, false, false, 0, false},
    {"RippleCfg", 0xBD, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"TimerH", 0xBE, "time", 3.2, false, "hr", true, 1, true, false, false, 0, false},
    {"Reserved", 0xBF, "", 0.00390625, false, "%", true, 6, true, false, false, 0, true},

    {"Rsense", 0xD0, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"ScOcvLim", 0xD1, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"Reserved", 0xD2, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"SOCHold", 0xD3, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"MaxPeakPower", 0xD4, "", 0.8, true, "", false, 0, true, false, false, 0, false},
    {"SusPeakPower", 0xD5, "", 0.8, true, "", false, 0, true, false, false, 0, false},
    {"PackResistance", 0xD6, "", 0.244141063, false, "", false, 0, true, false, false, 0, false},
    {"SysResistance", 0xD7, "", 0.244141063, false, "", false, 0, true, false, false, 0, false},
    {"MinSysVoltage", 0xD8, "", 0.000078125, false, "", false, 0, true, false, false, 0, false},
    {"MPPCurrent", 0xD9, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
    {"SPPCurrent", 0xDA, "current", 0.15625, true, "mA", true, 5, true, false, false, 0, false},
    {"ModelCfg", 0xDB, "", 1, false, "", false, 0, true, false, false, 0, false},
    {"AtQResidual", 0xDC, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},
    {"AtTTE", 0xDD, "time", 0.0015625, false, "hr", true, 6, true, false, false, 0, false},
    {"AtAvSOC", 0xDE, "percent", 0.00390625, false, "%", true, 6, true, false, false, 0, false},
    {"AtAvCap", 0xDF, "capacity", 0.5, false, "mAh", true, 1, true, false, false, 0, false},

    {"Reserved", 0xE0, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE1, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE2, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE3, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE4, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE5, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE6, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE7, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE8, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xE9, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xEA, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xEB, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xEC, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xED, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xEE, "voltage", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
    {"Reserved", 0xEF, "", 1, false, "", false, 0, true, false, false, 0, true},

    {"Reserved", 0xF0, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xF1, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xF2, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xF3, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xF4, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xF5, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xF6, "", 0.00390625, false, "%", true, 6, true, false, false, 0, true},
    {"Reserved", 0xF7, "", 0.001, true, "s", true, 3, true, false, false, 0, true},
    {"Reserved", 0xF8, "", 0.003051758, true, "C", true, 6, true, false, false, 0, true},
    {"Reserved", 0xF9, "", 0.00015625, false, "V", true, 6, true, false, false, 0, true},
    {"Reserved", 0xFA, "", 0.15625, true, "mA", true, 5, true, false, false, 0, true},
    {"Reserved", 0xFB, "", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
    {"Reserved", 0xFC, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xFD, "", 1, false, "", false, 0, true, false, false, 0, true},
    {"Reserved", 0xFE, "", 0.000078125, false, "V", true, 9, true, false, false, 0, true},
    {"Reserved", 0xFF, "", 0.00390625, false, "%", true, 6, true, false, false, 0, true},
};

void I2cDev_MAX17055::update() {
    uint8_t validity = 0;
    uint8_t batteryPercentage = 102;
    uint16_t voltage = 0;
    int32_t current = 0;
    getBatteryInfo(validity, batteryPercentage, voltage, current);

    // send local message
    BatteryStateMessage msg{validity, batteryPercentage, current >= 25, voltage};
    EventDispatcher::send_message(msg);
}

bool I2cDev_MAX17055::init(uint8_t addr_) {
    if (addr_ != I2CDEV_MAX17055_ADDR_1) return false;
    addr = addr_;
    model = I2CDEVMDL_MAX17055;
    query_interval = BATTERY_WIDGET_REFRESH_INTERVAL;
    if (detect()) {
        bool return_status = true;
        if (needsInitialization()) {
            // First-time or POR initialization
            return_status = full_reset_and_init();
        }
        partialInit();  // If you always want hibernation disabled
        // statusClear();  // I am not sure if this should be here or not (Clear all bits in the Status register (0x00))
        return return_status;
    }
    return false;
}

bool I2cDev_MAX17055::full_reset_and_init() {
    if (!soft_reset()) {
        return false;
    }
    if (!initialize_custom_parameters()) {
        return false;
    }
    if (!load_custom_parameters()) {
        return false;
    }
    if (!clear_por()) {
        return false;
    }
    return true;
}

bool I2cDev_MAX17055::soft_reset() {
    return write_register(0x0BB, 0x0000);
}

bool I2cDev_MAX17055::initialize_custom_parameters() {
    if (!write_register(0xD0, 0x03E8)) return false;  // Unknown register, possibly related to battery profile
    if (!write_register(0xDB, 0x0000)) return false;  // ModelCfg
    if (!write_register(0x05, 0x0000)) return false;  // RepCap
    if (!write_register(0x18, 0x1388)) return false;  // DesignCap
    if (!write_register(0x45, 0x009C)) return false;  // dQAcc
    if (!write_register(0x1E, 0x03C0)) return false;  // IChgTerm
    if (!write_register(0x3A, 0x9661)) return false;  // VEmpty
    if (!write_register(0x60, 0x0090)) return false;  // Unknown register
    if (!write_register(0x46, 0x0561)) return false;  // dPAcc
    if (!write_register(0xDB, 0x8000)) return false;  // ModelCfg

    if (!write_register(0x40, 0x0001)) return false;  // Set user mem to 1
    return true;
}

bool I2cDev_MAX17055::load_custom_parameters() {
    uint16_t hib_cfg = read_register(0xBA);
    if (!write_register(0xBA, 0x0000)) return false;  // Disable hibernate mode
    if (!write_register(0x60, 0x0000)) return false;  // Unknown register

    // Wait for the model to be loaded
    for (int i = 0; i < 10; i++) {
        uint16_t model_cfg = read_register(0xDB);
        if ((model_cfg & 0x8000) == 0) {
            break;
        }
        chThdSleepMilliseconds(10);
    }

    if (!write_register(0xBA, hib_cfg)) return false;  // Restore hibernate config
    return true;
}

bool I2cDev_MAX17055::clear_por() {
    uint16_t status = read_register(0x00);
    status &= ~(1 << 1);
    return write_register(0x00, status);
}

bool I2cDev_MAX17055::needsInitialization() {
    uint16_t UserMem1 = read_register(0x40);

    if (UserMem1 == 0) {
        return true;
    }
    return false;
}

void I2cDev_MAX17055::partialInit() {
    // Only update necessary volatile settings
    setHibCFG(0x0000);  // If you always want hibernation disabled
    // Add any other volatile settings that need updating
}

bool I2cDev_MAX17055::reset_learned() {
    // this if for reset all the learned parameters by ic
    // the full inis should do this
    return full_reset_and_init();
}

bool I2cDev_MAX17055::detect() {
    // Read the DevName register (0x21)
    uint16_t dev_name = read_register(0x21);

    // The DevName register should return 0x4010 for MAX17055
    if (dev_name == 0x4010) {
        return true;
    }

    // If DevName doesn't match, try reading Status register as a fallback
    uint16_t status = read_register(0x00);
    if (status != 0xFFFF && status != 0x0000) {
        return true;
    }

    return false;
}

const I2cDev_MAX17055::RegisterEntry* I2cDev_MAX17055::findEntry(const char* name) const {
    for (const auto& entry : entries) {
        if (std::strcmp(entry.name, name) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

uint16_t I2cDev_MAX17055::read_register(const uint8_t reg) {
    const std::array<uint8_t, 1> tx{reg};
    std::array<uint8_t, 2> rx{0x00, 0x00};

    i2cbus.transmit(addr, tx.data(), tx.size());
    i2cbus.receive(addr, rx.data(), rx.size());

    // Combine the two bytes into a 16-bit value
    // little-endian format (LSB first)
    return static_cast<uint16_t>((rx[1] << 8) | rx[0]);
}

bool I2cDev_MAX17055::write_register(const uint8_t reg, const uint16_t value) {
    std::array<uint8_t, 3> tx;
    tx[0] = reg;
    tx[1] = value & 0xFF;         // Low byte
    tx[2] = (value >> 8) & 0xFF;  // High byte

    bool success = i2cbus.transmit(addr, tx.data(), tx.size());
    chThdSleepMilliseconds(1);
    return success;
}

void I2cDev_MAX17055::getBatteryInfo(uint8_t& valid_mask, uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current) {
    // Read Status Register
    uint16_t status = read_register(0x00);
    voltage = averageMVoltage();
    if ((status == 0 && voltage == 0) || (status == 0x0002 && voltage == 3600) || (status == 0x0002 && voltage == 0)) {
        valid_mask = 0;
        return;
    }
    batteryPercentage = stateOfCharge();
    current = instantCurrent();
    valid_mask = 31;  // BATT_VALID_VOLTAGE + CURRENT + PERCENT + BATT_VALID_CYCLES + BATT_VALID_TTEF
    if (battery::BatteryManagement::calcOverride) {
        valid_mask &= ~battery::BatteryManagement::BATT_VALID_PERCENT;  // indicate it is voltage based
        batteryPercentage = battery::BatteryManagement::calc_percent_voltage(voltage);
    }
}

float I2cDev_MAX17055::getValue(const char* entityName) {
    const RegisterEntry* entry = findEntry(entityName);
    if (entry) {
        uint16_t raw_value = read_register(entry->address);

        float scaled_value;
        if (entry->is_signed) {
            int16_t signed_value = static_cast<int16_t>(raw_value);
            scaled_value = signed_value * entry->scalar;
        } else {
            scaled_value = raw_value * entry->scalar;
        }

        return scaled_value;
    }
    return 0;  // Return 0 if entry not found
}

uint16_t I2cDev_MAX17055::averageMVoltage(void) {
    return static_cast<uint16_t>(getValue("AvgVCell") * 1000.0f);  // Convert to millivolts
}

int32_t I2cDev_MAX17055::instantCurrent(void) {
    return getValue("Current");

    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x0A);

    // Convert to signed int16_t (two's complement)
    int32_t _Signed_Raw = static_cast<int16_t>(_Measurement_Raw);

    int32_t _Value = (_Signed_Raw * 15625) / (__MAX17055_Resistor__ * 100) / 100000;

    // End Function
    return _Value;
}

uint16_t I2cDev_MAX17055::stateOfCharge(void) {
    return getValue("RepSOC");
}

bool I2cDev_MAX17055::setEmptyVoltage(uint16_t _Empty_Voltage) {
    // Calculate the new VE_Empty value (upper 9 bits)
    uint16_t ve_empty = ((_Empty_Voltage * 100) / 10) & 0xFF80;

    // Read the current register value
    uint16_t current_value = read_register(0x3A);

    // Preserve the lower 7 bits (VR_Empty) and combine with new VE_Empty
    uint16_t new_value = (ve_empty & 0xFF80) | (current_value & 0x007F);

    // Write the new value back to the register
    return write_register(0x3A, new_value);
}

bool I2cDev_MAX17055::setRecoveryVoltage(uint16_t _Recovery_Voltage) {
    // Calculate the new VR_Empty value (lower 7 bits)
    uint16_t vr_empty = (_Recovery_Voltage * 25) & 0x007F;  // 40mV per bit, 25 = 1000/40

    // Read the current register value
    uint16_t current_value = read_register(0x3A);

    // Preserve the upper 9 bits (VE_Empty) and combine with new VR_Empty
    uint16_t new_value = (current_value & 0xFF80) | vr_empty;

    // Write the new value back to the register
    return write_register(0x3A, new_value);
}

bool I2cDev_MAX17055::setMinVoltage(uint16_t _Minimum_Voltage) {
    uint16_t current_value = read_register(0x01);

    uint16_t min_voltage_raw = (_Minimum_Voltage * 50) & 0x00FF;  // 20mV per bit, 50 = 1000/20
    uint16_t new_value = (current_value & 0xFF00) | min_voltage_raw;

    return write_register(0x01, new_value);
}

bool I2cDev_MAX17055::setMaxVoltage(uint16_t _Maximum_Voltage) {
    uint16_t current_value = read_register(0x01);

    uint16_t max_voltage_raw = ((_Maximum_Voltage * 50) & 0x00FF) << 8;  // 20mV per bit, 50 = 1000/20
    uint16_t new_value = (current_value & 0x00FF) | max_voltage_raw;

    return write_register(0x01, new_value);
}

bool I2cDev_MAX17055::setMaxCurrent(uint16_t _Maximum_Current) {
    uint16_t current_value = read_register(0xB4);

    uint16_t max_current_raw = ((_Maximum_Current * 25) & 0x00FF) << 8;  // 40mV per bit, 25 = 1000/40
    uint16_t new_value = (current_value & 0x00FF) | max_current_raw;

    return write_register(0xB4, new_value);
}

bool I2cDev_MAX17055::setChargeTerminationCurrent(uint16_t _Charge_Termination_Current) {
    float lsb_mA = 1.5625 / (__MAX17055_Resistor__ * 1000);  // Convert to mA
    uint16_t ichgterm_value = static_cast<uint16_t>(round(_Charge_Termination_Current / lsb_mA));

    return write_register(0x1E, ichgterm_value);
}

bool I2cDev_MAX17055::setDesignCapacity(const uint16_t _Capacity) {
    uint16_t raw_cap = (uint16_t)_Capacity * 2;
    return write_register(0x18, raw_cap);
}

bool I2cDev_MAX17055::setFullCapRep(const uint16_t _Capacity) {
    uint16_t raw_cap = _Capacity * 2;  // 0.5mAh per LSB
    return write_register(0x10, raw_cap);
}

bool I2cDev_MAX17055::setFullCapNom(const uint16_t _Capacity) {
    uint16_t raw_cap = _Capacity * 2;  // 0.5mAh per LSB
    return write_register(0x23, raw_cap);
}

bool I2cDev_MAX17055::setRepCap(const uint16_t _Capacity) {
    uint16_t raw_cap = _Capacity * 2;  // 0.5mAh per LSB
    return write_register(0x05, raw_cap);
}

bool I2cDev_MAX17055::setMinSOC(uint8_t _Minimum_SOC) {
    uint16_t current_value = read_register(0x03);

    uint16_t min_soc_raw = ((_Minimum_SOC * 256) / 100) & 0x00FF;
    uint16_t new_value = (current_value & 0xFF00) | min_soc_raw;

    return write_register(0x03, new_value);
}

bool I2cDev_MAX17055::setMaxSOC(uint8_t _Maximum_SOC) {
    uint16_t current_value = read_register(0x03);

    uint16_t max_soc_raw = (((_Maximum_SOC * 256) / 100) & 0x00FF) << 8;
    uint16_t new_value = (current_value & 0x00FF) | max_soc_raw;

    return write_register(0x03, new_value);
}

bool I2cDev_MAX17055::setMinTemperature(uint8_t _Minimum_Temperature) {
    uint16_t current_value = read_register(0x02);

    uint16_t min_temp_raw = (uint8_t)_Minimum_Temperature;
    uint16_t new_value = (current_value & 0xFF00) | min_temp_raw;

    return write_register(0x02, new_value);
}

bool I2cDev_MAX17055::setMaxTemperature(uint8_t _Maximum_Temperature) {
    uint16_t current_value = read_register(0x02);

    uint16_t max_temp_raw = ((uint8_t)_Maximum_Temperature) << 8;
    uint16_t new_value = (current_value & 0x00FF) | max_temp_raw;

    return write_register(0x02, new_value);
}

bool I2cDev_MAX17055::setModelCfg(const uint8_t _Model_ID) {
    uint16_t model_cfg = 0x0400;  // Set Charge Voltage (bit 10)

    // Set Battery Model
    switch (_Model_ID) {
        case 0:  // Default model
            break;
        case 2:  // EZ model
            model_cfg |= 0x0020;
            break;
        case 6:  // Short EZ model
            model_cfg |= 0x0060;
            break;
        default:
            return false;  // Invalid model ID
    }

    return write_register(0xDB, model_cfg);
}

bool I2cDev_MAX17055::setHibCFG(const uint16_t _Config) {
    return write_register(0xBA, _Config);
}

void I2cDev_MAX17055::config(void) {
    uint16_t config1 = 0x0000;
    uint16_t config2 = 0x0618;  // Default value: 0b00011000 00000110

    // Set Configuration bits [Config1]
    if (MAX17055_Ber) config1 |= 0x0001;
    if (MAX17055_Bei) config1 |= 0x0002;
    if (MAX17055_Aen) config1 |= 0x0004;
    if (MAX17055_FTHRM) config1 |= 0x0008;
    if (MAX17055_ETHRM) config1 |= 0x0010;
    if (MAX17055_COMMSH) config1 |= 0x0040;
    if (MAX17055_SHDN) config1 |= 0x0080;
    if (MAX17055_Tex) config1 |= 0x0100;
    if (MAX17055_Ten) config1 |= 0x0200;
    if (MAX17055_AINSH) config1 |= 0x0400;
    if (MAX17055_IS) config1 |= 0x0800;
    if (MAX17055_VS) config1 |= 0x1000;
    if (MAX17055_TS) config1 |= 0x2000;
    if (MAX17055_SS) config1 |= 0x4000;
    if (MAX17055_TSel) config1 |= 0x8000;

    // Set Configuration bits [Config2]
    if (MAX17055_CPMode) config2 |= 0x0002;
    if (MAX17055_LDMDL) config2 |= 0x0020;
    if (MAX17055_TAIrtEN) config2 |= 0x0040;
    if (MAX17055_dSOCen) config2 |= 0x0080;
    if (MAX17055_DPEn) config2 |= 0x1000;
    if (MAX17055_AtRateEn) config2 |= 0x2000;

    // Write configurations to registers
    write_register(0x1D, config1);
    write_register(0xBB, config2);
}

bool I2cDev_MAX17055::statusClear() {
    // Clear all bits in the Status register (0x00)
    return write_register(0x00, 0x0000);
}

bool bitRead(uint8_t value, uint8_t bit) {
    return (value >> bit) & 0x01;
}

bool I2cDev_MAX17055::statusControl(const uint8_t _Status) {
    // Read Status Register (0x00)
    uint16_t status_register = read_register(0x00);

    // Control for Status
    switch (_Status) {
        case MAX17055_POR:
            return bitRead(status_register & 0xFF, 1);
        case MAX17055_IMin:
            return bitRead(status_register & 0xFF, 2);
        case MAX17055_IMax:
            return bitRead(status_register & 0xFF, 6);
        case MAX17055_SOC_Change:
            return bitRead(status_register & 0xFF, 7);
        case MAX17055_Bat_Status:
            return bitRead(status_register & 0xFF, 3);
        case MAX17055_VMin:
            return bitRead(status_register >> 8, 0);
        case MAX17055_VMax:
            return bitRead(status_register >> 8, 4);
        case MAX17055_TMin:
            return bitRead(status_register >> 8, 1);
        case MAX17055_TMax:
            return bitRead(status_register >> 8, 5);
        case MAX17055_SOC_Min:
            return bitRead(status_register >> 8, 2);
        case MAX17055_SOC_Max:
            return bitRead(status_register >> 8, 6);
        case MAX17055_Bat_Insert:
            return bitRead(status_register >> 8, 3);
        case MAX17055_Bat_Remove:
            return bitRead(status_register >> 8, 7);
        default:
            return false;
    }
}

} /* namespace i2cdev */
