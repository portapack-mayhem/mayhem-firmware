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
#include "max17055.hpp"
#include "utility.hpp"
#include "portapack_persistent_memory.hpp"
#include <cstring>
#include <algorithm>
#include <cstdint>

namespace battery {
namespace max17055 {

void MAX17055::init() {
    if (!detected_) {
        detected_ = detect();
    }
    if (detected_) {
        // uint16_t status = getStatus();
        // bool isPOR = status & 0x0002;  // Check POR bit

        // if (isPOR || !persistent_memory::sui_charge_ic_initialized()) {
        // if (!portapack::persistent_memory::charge_ic_initialized()) {

        if (needsInitialization()) {
            // First-time or POR initialization
            fullInit();
        } else {
            // Subsequent boot
            partialInit();
        }

        statusClear();
    }
}

bool MAX17055::needsInitialization() {
    // uint16_t designCap = read_register(0x18);
    // uint16_t vEmpty = read_register(0x3A);
    // uint16_t iChgTerm = read_register(0x1E);

    uint16_t designCap = designCapacity();
    uint16_t vEmpty = emptyVoltage();
    uint16_t iChgTerm = chargeTerminationCurrent();

    // Compare with expected values
    if (designCap != __MAX17055_Design_Capacity__ ||
        vEmpty != __MAX17055_Empty_Voltage__ ||
        iChgTerm != __MAX17055_Termination_Current__) {
        return true;
    }
    return false;
}

void MAX17055::fullInit() {
    config();
    setHibCFG(0x0000);
    setDesignCapacity(__MAX17055_Design_Capacity__);
    setModelCfg(__MAX17055_Battery_Model__);
    setChargeTerminationCurrent(__MAX17055_Termination_Current__);
    setEmptyVoltage(__MAX17055_Empty_Voltage__);
    setRecoveryVoltage(__MAX17055_Recovery_Voltage__);
    setMinVoltage(__MAX17055_Min_Voltage__);
    setMaxVoltage(__MAX17055_Max_Voltage__);
    setMaxCurrent(__MAX17055_Max_Current__);
    setMinSOC(__MAX17055_Min_SOC__);
    setMaxSOC(__MAX17055_Max_SOC__);
    setMinTemperature(__MAX17055_Min_Temperature__);
    setMaxTemperature(__MAX17055_Max_Temperature__);
}

void MAX17055::partialInit() {
    // Only update necessary volatile settings
    setHibCFG(0x0000);  // If you always want hibernation disabled
    // Add any other volatile settings that need updating
}

bool MAX17055::reset_learned() {
    // this if for reset all the learned parameters by ic
    // the full inis should do this
    fullInit();
    return true;
}

bool MAX17055::detect() {
    // Read the DevName register (0x21)
    uint16_t dev_name = read_register(0x21);

    // The DevName register should return 0x4010 for MAX17055
    if (dev_name == 0x4010) {
        detected_ = true;
        return true;
    }

    // If DevName doesn't match, try reading Status register as a fallback
    uint16_t status = read_register(0x00);
    if (status != 0xFFFF && status != 0x0000) {
        detected_ = true;
        return true;
    }

    detected_ = false;
    return false;
}

bool bitRead(uint8_t value, uint8_t bit) {
    return (value >> bit) & 0x01;
}

void bitSet(uint16_t& value, uint8_t bit) {
    value |= (1 << bit);
}

void bitSet(uint8_t& value, uint8_t bit) {
    value |= (1 << bit);
}

void bitClear(uint16_t& value, uint8_t bit) {
    value &= ~(1 << bit);
}

uint16_t MAX17055::read_register(const uint8_t reg) {
    const std::array<uint8_t, 1> tx{reg};
    std::array<uint8_t, 2> rx{0x00, 0x00};

    bus.transmit(bus_address, tx.data(), tx.size());
    bus.receive(bus_address, rx.data(), rx.size());

    // Combine the two bytes into a 16-bit value
    // little-endian format (LSB first)
    return static_cast<uint16_t>((rx[1] << 8) | rx[0]);
}

bool MAX17055::write_register(const uint8_t reg, const uint16_t value) {
    std::array<uint8_t, 3> tx;
    tx[0] = reg;
    tx[1] = value & 0xFF;         // Low byte
    tx[2] = (value >> 8) & 0xFF;  // High byte

    bool success = bus.transmit(bus_address, tx.data(), tx.size());
    chThdSleepMilliseconds(1);
    return success;
}

void MAX17055::getBatteryInfo(uint8_t& valid_mask, uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current) {
    if (detected_) {
        // Read Status Register
        uint16_t status = read_register(0x00);
        voltage = averageVoltage();
        if ((status == 0 && voltage == 0) || (status == 0x0002 && voltage == 3600) || (status == 0x0002 && voltage == 0)) {
            valid_mask = 0;
            return;
        }
        batteryPercentage = stateOfCharge();
        current = instantCurrent();
        valid_mask = 7;  // BATT_VALID_VOLTAGE + CURRENT + PERCENT
    } else {
        // let's indicate the data is wrong. ui will handle this by display UNK values.
        valid_mask = 0;
    }
}

bool MAX17055::setEmptyVoltage(uint16_t _Empty_Voltage) {
    // Calculate the new VE_Empty value (upper 9 bits)
    uint16_t ve_empty = ((_Empty_Voltage * 100) / 10) & 0xFF80;

    // Read the current register value
    uint16_t current_value = read_register(0x3A);

    // Preserve the lower 7 bits (VR_Empty) and combine with new VE_Empty
    uint16_t new_value = (ve_empty & 0xFF80) | (current_value & 0x007F);

    // Write the new value back to the register
    return write_register(0x3A, new_value);
}

bool MAX17055::setRecoveryVoltage(uint16_t _Recovery_Voltage) {
    // Calculate the new VR_Empty value (lower 7 bits)
    uint16_t vr_empty = (_Recovery_Voltage * 25) & 0x007F;  // 40mV per bit, 25 = 1000/40

    // Read the current register value
    uint16_t current_value = read_register(0x3A);

    // Preserve the upper 9 bits (VE_Empty) and combine with new VR_Empty
    uint16_t new_value = (current_value & 0xFF80) | vr_empty;

    // Write the new value back to the register
    return write_register(0x3A, new_value);
}

bool MAX17055::setMinVoltage(uint16_t _Minimum_Voltage) {
    uint16_t current_value = read_register(0x01);

    uint16_t min_voltage_raw = (_Minimum_Voltage * 50) & 0x00FF;  // 20mV per bit, 50 = 1000/20
    uint16_t new_value = (current_value & 0xFF00) | min_voltage_raw;

    return write_register(0x01, new_value);
}

bool MAX17055::setMaxVoltage(uint16_t _Maximum_Voltage) {
    uint16_t current_value = read_register(0x01);

    uint16_t max_voltage_raw = ((_Maximum_Voltage * 50) & 0x00FF) << 8;  // 20mV per bit, 50 = 1000/20
    uint16_t new_value = (current_value & 0x00FF) | max_voltage_raw;

    return write_register(0x01, new_value);
}

bool MAX17055::setMaxCurrent(uint16_t _Maximum_Current) {
    uint16_t current_value = read_register(0xB4);

    uint16_t max_current_raw = ((_Maximum_Current * 25) & 0x00FF) << 8;  // 40mV per bit, 25 = 1000/40
    uint16_t new_value = (current_value & 0x00FF) | max_current_raw;

    return write_register(0xB4, new_value);
}

bool MAX17055::setChargeTerminationCurrent(uint16_t _Charge_Termination_Current) {
    float lsb_mA = 1.5625 / (__MAX17055_Resistor__ * 1000);  // Convert to mA
    uint16_t ichgterm_value = static_cast<uint16_t>(round(_Charge_Termination_Current / lsb_mA));

    return write_register(0x1E, ichgterm_value);
}

bool MAX17055::setDesignCapacity(const uint16_t _Capacity) {
    uint16_t raw_cap = (uint16_t)_Capacity * 2;
    return write_register(0x18, raw_cap);
}

bool MAX17055::setFullCapRep(const uint16_t _Capacity) {
    uint16_t raw_cap = _Capacity * 2;  // 0.5mAh per LSB
    return write_register(0x10, raw_cap);
}

bool MAX17055::setFullCapNom(const uint16_t _Capacity) {
    uint16_t raw_cap = _Capacity * 2;  // 0.5mAh per LSB
    return write_register(0x23, raw_cap);
}

bool MAX17055::setRepCap(const uint16_t _Capacity) {
    uint16_t raw_cap = _Capacity * 2;  // 0.5mAh per LSB
    return write_register(0x05, raw_cap);
}

bool MAX17055::setMinSOC(uint8_t _Minimum_SOC) {
    uint16_t current_value = read_register(0x03);

    uint16_t min_soc_raw = ((_Minimum_SOC * 256) / 100) & 0x00FF;
    uint16_t new_value = (current_value & 0xFF00) | min_soc_raw;

    return write_register(0x03, new_value);
}

bool MAX17055::setMaxSOC(uint8_t _Maximum_SOC) {
    uint16_t current_value = read_register(0x03);

    uint16_t max_soc_raw = (((_Maximum_SOC * 256) / 100) & 0x00FF) << 8;
    uint16_t new_value = (current_value & 0x00FF) | max_soc_raw;

    return write_register(0x03, new_value);
}

bool MAX17055::setMinTemperature(uint8_t _Minimum_Temperature) {
    uint16_t current_value = read_register(0x02);

    uint16_t min_temp_raw = (uint8_t)_Minimum_Temperature;
    uint16_t new_value = (current_value & 0xFF00) | min_temp_raw;

    return write_register(0x02, new_value);
}

bool MAX17055::setMaxTemperature(uint8_t _Maximum_Temperature) {
    uint16_t current_value = read_register(0x02);

    uint16_t max_temp_raw = ((uint8_t)_Maximum_Temperature) << 8;
    uint16_t new_value = (current_value & 0x00FF) | max_temp_raw;

    return write_register(0x02, new_value);
}

bool MAX17055::setModelCfg(const uint8_t _Model_ID) {
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

bool MAX17055::setHibCFG(const uint16_t _Config) {
    return write_register(0xBA, _Config);
}

void MAX17055::config(void) {
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

uint16_t MAX17055::instantVoltage(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x09);

    // Calculate Measurement
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 1.25 / 16);

    // End Function
    return _Value;
}

uint16_t MAX17055::averageVoltage(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x19);

    // Calculate Measurement
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 1.25 / 16);

    // End Function
    return _Value;
}

uint16_t MAX17055::emptyVoltage(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x3A);
    _Measurement_Raw = ((_Measurement_Raw & 0xFF80) >> 7);

    // Calculate Measurement
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 10);

    // End Function
    return _Value;
}

uint16_t MAX17055::recoveryVoltage(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x3A);

    _Measurement_Raw = (_Measurement_Raw & 0x7F);

    // Calculate Measurement
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 40);

    // End Function
    return _Value;
}

int32_t MAX17055::instantCurrent(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x0A);

    // Convert to signed int16_t (two's complement)
    int32_t _Signed_Raw = static_cast<int16_t>(_Measurement_Raw);

    int32_t _Value = (_Signed_Raw * 15625) / (__MAX17055_Resistor__ * 100) / 100000;

    // End Function
    return _Value;
}

int32_t MAX17055::averageCurrent(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x0B);

    // Convert to signed int16_t (two's complement)
    int32_t _Signed_Raw = static_cast<int16_t>(_Measurement_Raw);

    int32_t _Value = (_Signed_Raw * 15625) / (__MAX17055_Resistor__ * 100) / 100000;

    // End Function
    return _Value;
}

uint16_t MAX17055::stateOfCharge(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x06);  // RepSOC

    // Calculate Measurement
    uint8_t _Value = (_Measurement_Raw >> 8) & 0xFF;

    // End Function
    return _Value;
}

uint16_t MAX17055::averageStateOfCharge(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x0E);

    // Calculate Measurement
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 100 / 256);

    // End Function
    return _Value;
}

uint16_t MAX17055::instantCapacity(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x05);

    // Calculate Data
    uint16_t _Value = _Measurement_Raw * 5 / 1000 / __MAX17055_Resistor__;

    // End Function
    return _Value;
}

uint16_t MAX17055::designCapacity(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x18);
    // End Function
    return _Measurement_Raw / 2;
}

uint16_t MAX17055::fullCapacity(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x35);

    // Calculate Data
    uint16_t _Value = _Measurement_Raw * 5 / 1000 / __MAX17055_Resistor__;

    // End Function
    return _Value;
}

uint16_t MAX17055::icTemperature(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x08);

    // Declare Variables
    bool _Signiture = false;

    // Control for Negative Value
    if ((_Measurement_Raw >> 12) == 0xF) {
        // Calculate Data
        _Measurement_Raw = 0xFFFF - _Measurement_Raw;

        // Assign Signiture
        _Signiture = true;
    }

    // Calculate Data
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 100 / 256);

    // Assign Signiture
    if (_Signiture) _Value = -_Value;

    // End Function
    return _Value;
}

uint16_t MAX17055::timeToEmpty(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x11);

    // Calculate Data
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 5625 / 60 / 60 / 100);

    // End Function
    return _Value;
}

uint16_t MAX17055::timeToFull(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x20);

    // Calculate Data
    uint16_t _Value = ((uint32_t)_Measurement_Raw * 5625 / 60 / 60 / 100);

    // End Function
    return _Value;
}

uint16_t MAX17055::batteryAge(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x07);

    // End Function
    return _Measurement_Raw;
}

uint16_t MAX17055::chargeCycle(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x17);

    // End Function
    return _Measurement_Raw;
}

bool MAX17055::statusControl(const uint8_t _Status) {
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

bool MAX17055::statusClear() {
    // Clear all bits in the Status register (0x00)
    return write_register(0x00, 0x0000);
}

uint16_t MAX17055::chargeTerminationCurrent(void) {
    uint16_t _Measurement_Raw = read_register(0x1E);
    float lsb_mA = 1.5625 / (__MAX17055_Resistor__ * 1000);  // Convert to mA
    uint16_t Value = static_cast<uint16_t>(round(_Measurement_Raw * lsb_mA));
    return Value;
}

} /* namespace max17055 */
}  // namespace battery