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
#include <cstring>
#include <algorithm>
#include <cstdint>

namespace battery {
namespace max17055 {

void MAX17055::init() {
    if (!detected_) {
        detected_ = detect();
    } else {
        config();
        setHibCFG(0x0000);

        // Design Capacity Register
        setDesignCapacity(__MAX17055_Design_Capacity__);

        // ModelCfg Register
        setModelCfg(__MAX17055_Battery_Model__);

        // IChgTerm Register
        setChargeTerminationCurrent(__MAX17055_Termination_Current__);

        // VEmpty Register
        setEmptyVoltage(__MAX17055_Empty_Voltage__);

        // VRecovery Register
        setRecoveryVoltage(__MAX17055_Recovery_Voltage__);

        // Set Minimum Voltage
        setMinVoltage(__MAX17055_Min_Voltage__);

        // Set Maximum Voltage
        setMaxVoltage(__MAX17055_Max_Voltage__);

        // Set Maximum Current
        setMaxCurrent(__MAX17055_Max_Current__);

        // Set Minimum SOC
        setMinSOC(__MAX17055_Min_SOC__);

        // Set Maximum SOC
        setMaxSOC(__MAX17055_Max_SOC__);

        // Set Minimum Temperature
        setMinTemperature(__MAX17055_Min_Temperature__);

        // Set Maximum Temperature
        setMaxTemperature(__MAX17055_Max_Temperature__);

        // Clear Bits
        statusClear();
    }
}

bool MAX17055::detect() {
    uint8_t _MAX17055_Data[2];

    // Get Data from IC
    if (readMultipleRegister(0x00, _MAX17055_Data, 2, false)) {
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
    return success;
}

bool MAX17055::readMultipleRegister(uint8_t reg, uint8_t* data, uint8_t length, bool endTransmission) {
    if (bus.transmit(bus_address, &reg, 1)) {
        if (bus.receive(bus_address, data, length)) {
            if (endTransmission) {
                // bus.stop();  // Testing if we need this line
                // Perform any necessary end transmission actions
                // For example, you can use bus.endTransmission()
            }
            return true;
        }
    }
    return false;
}

bool MAX17055::writeMultipleRegister(uint8_t reg, const uint8_t* data, uint8_t length) {
    uint8_t buffer[length + 1];
    buffer[0] = reg;
    memcpy(buffer + 1, data, length);

    if (bus.transmit(bus_address, buffer, length + 1)) {
        // Perform any necessary end transmission actions
        // For example, you can use bus.endTransmission()
        return true;
    }
    return false;
}

void MAX17055::getBatteryInfo(uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current) {
    voltage = averageVoltage();
    batteryPercentage = stateOfCharge();
    current = instantCurrent();
}

bool MAX17055::setEmptyVoltage(uint16_t _Empty_Voltage) {
    // Set Voltage Value
    uint16_t _Raw_Voltage = ((uint16_t)((uint16_t)_Empty_Voltage * 100) << 7) & 0b1111111110000000;

    // Define Data Variable
    uint8_t MAX17055_RAW_Data[2];

    // Handle Data
    MAX17055_RAW_Data[1] = (uint8_t)(_Raw_Voltage >> 8);
    MAX17055_RAW_Data[0] = (uint8_t)(_Raw_Voltage & 0x00FF);

    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x3A, MAX17055_Current_Data, 2, true);

    // Clear Current Value
    MAX17055_Current_Data[0] &= 0b01111111;
    MAX17055_Current_Data[1] &= 0b00000000;

    // Define Data Variable
    uint8_t MAX17055_Handle_Data[2];

    // Handle Data
    MAX17055_Handle_Data[0] = MAX17055_Current_Data[0] | MAX17055_RAW_Data[0];
    MAX17055_Handle_Data[1] = MAX17055_Current_Data[1] | MAX17055_RAW_Data[1];

    // Set Register
    bool _Result = writeMultipleRegister(0x3A, MAX17055_Handle_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setRecoveryVoltage(uint16_t _Recovery_Voltage) {
    // Handle Value
    _Recovery_Voltage = _Recovery_Voltage * 1000 / 40;

    // Set Voltage Value
    uint16_t _Raw_Voltage = ((uint16_t)_Recovery_Voltage);

    // Define Data Variable
    uint8_t MAX17055_RAW_Data[2];

    // Handle Data
    MAX17055_RAW_Data[1] = (uint8_t)(_Raw_Voltage >> 8);
    MAX17055_RAW_Data[0] = (uint8_t)(_Raw_Voltage & 0x00FF);

    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x3A, MAX17055_Current_Data, 2, true);

    // Clear Current Value
    MAX17055_Current_Data[0] &= 0b10000000;
    MAX17055_Current_Data[1] &= 0b11111111;

    // Define Data Variable
    uint8_t MAX17055_Handle_Data[2];

    // Handle Data
    MAX17055_Handle_Data[0] = MAX17055_Current_Data[0] | MAX17055_RAW_Data[0];
    MAX17055_Handle_Data[1] = MAX17055_Current_Data[1] | MAX17055_RAW_Data[1];

    // Set Register
    bool _Result = writeMultipleRegister(0x3A, MAX17055_Handle_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMinVoltage(uint16_t _Minimum_Voltage) {
    // Set Voltage Value
    uint8_t _Raw_Voltage = (uint8_t)(_Minimum_Voltage * 1000 / 20);

    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x01, MAX17055_Current_Data, 2, true);

    // Set Voltage Value
    MAX17055_Current_Data[0] = _Raw_Voltage;

    // Set Register
    bool _Result = writeMultipleRegister(0x01, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMaxVoltage(uint16_t _Maximum_Voltage) {
    // Set Voltage Value
    uint8_t _Raw_Voltage = (uint8_t)(_Maximum_Voltage * 1000 / 20);

    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x01, MAX17055_Current_Data, 2, true);

    // Set Voltage Value
    MAX17055_Current_Data[1] = _Raw_Voltage;

    // Set Register
    bool _Result = writeMultipleRegister(0x01, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMaxCurrent(uint16_t _Maximum_Current) {
    // Set Current Value
    uint8_t _Raw_Current = (uint8_t)(_Maximum_Current * 1000 / 40);

    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0xB4, MAX17055_Current_Data, 2, true);

    // Set Current Value
    MAX17055_Current_Data[1] = _Raw_Current;

    // Set Register
    bool _Result = writeMultipleRegister(0xB4, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setChargeTerminationCurrent(uint16_t _Charge_Termination_Current) {
    // Handle Raw Data
    uint16_t _RAW_Data = (uint16_t)(_Charge_Termination_Current * 1000000 * __MAX17055_Resistor__ / 1.5625);

    // Declare Default Data Array
    uint8_t _Data[2];

    // Set Data Low/High Byte
    _Data[0] = ((_RAW_Data & (uint16_t)0x00FF));
    _Data[1] = ((_RAW_Data & (uint16_t)0xFF00) >> 8);

    // Set Register
    bool _Result = writeMultipleRegister(0x1E, _Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setDesignCapacity(const uint16_t _Capacity) {
    // Set Raw
    uint16_t _Raw_Cap = (uint16_t)_Capacity * 2;

    // Declare Default Data Array
    uint8_t _Data[2];

    // Set Data Low/High Byte
    _Data[0] = ((_Raw_Cap & (uint16_t)0x00FF));
    _Data[1] = ((_Raw_Cap & (uint16_t)0xFF00) >> 8);

    // Set Register
    bool _Result = writeMultipleRegister(0x18, _Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMinSOC(uint8_t _Minimum_SOC) {
    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x03, MAX17055_Current_Data, 2, true);

    // Scale Value
    uint8_t _MinSOC = (_Minimum_SOC / 100) * 255;

    // Set Voltage Value
    MAX17055_Current_Data[0] = _MinSOC;

    // Set Register
    bool _Result = writeMultipleRegister(0x03, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMaxSOC(uint8_t _Maximum_SOC) {
    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x03, MAX17055_Current_Data, 2, true);

    // Scale Value
    uint8_t _MaxSOC = (_Maximum_SOC / 100) * 255;

    // Set Voltage Value
    MAX17055_Current_Data[1] = _MaxSOC;

    // Set Register
    bool _Result = writeMultipleRegister(0x03, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMinTemperature(uint8_t _Minimum_Temperature) {
    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x02, MAX17055_Current_Data, 2, true);

    // Set Voltage Value
    MAX17055_Current_Data[0] = _Minimum_Temperature;

    // Set Register
    bool _Result = writeMultipleRegister(0x02, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setMaxTemperature(uint8_t _Maximum_Temperature) {
    // Define Data Variable
    uint8_t MAX17055_Current_Data[2];

    // Read Current Register
    readMultipleRegister(0x02, MAX17055_Current_Data, 2, true);

    // Set Voltage Value
    MAX17055_Current_Data[1] = _Maximum_Temperature;

    // Set Register
    bool _Result = writeMultipleRegister(0x02, MAX17055_Current_Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setModelCfg(const uint8_t _Model_ID) {
    // Declare Variable
    uint16_t _Data_Set = 0x00;

    // Set Charge Voltage
    bitSet(_Data_Set, 10);

    // Set Battery Model
    if (_Model_ID == 0) {
        bitClear(_Data_Set, 4);
        bitClear(_Data_Set, 5);
        bitClear(_Data_Set, 6);
        bitClear(_Data_Set, 7);
    }
    if (_Model_ID == 2) {
        bitClear(_Data_Set, 4);
        bitSet(_Data_Set, 5);
        bitClear(_Data_Set, 6);
        bitClear(_Data_Set, 7);
    }
    if (_Model_ID == 6) {
        bitClear(_Data_Set, 4);
        bitSet(_Data_Set, 5);
        bitSet(_Data_Set, 6);
        bitClear(_Data_Set, 7);
    }

    // Declare Default Data Array
    uint8_t _Data[2];

    // Set Data Low/High Byte
    _Data[0] = ((_Data_Set & (uint16_t)0x00FF));
    _Data[1] = ((_Data_Set & (uint16_t)0xFF00) >> 8);

    // Set Register
    bool _Result = writeMultipleRegister(0xDB, _Data, 2);

    // End Function
    return _Result;
}

bool MAX17055::setHibCFG(const uint16_t _Config) {
    // Declare Default Data Array
    uint8_t _Data[2];

    // Set Data Low/High Byte
    _Data[0] = ((_Config & (uint16_t)0x00FF));
    _Data[1] = ((_Config & (uint16_t)0xFF00) >> 8);

    // Set Register
    bool _Result = writeMultipleRegister(0xBA, _Data, 2);

    // End Function
    return _Result;
}

void MAX17055::config(void) {
    // Declare Default Data Array
    uint8_t _Config1[2];
    uint8_t _Config2[2];

    // Set Default Data
    _Config1[0] = 0b00000000;
    _Config1[1] = 0b00000000;
    _Config2[0] = 0b00011000;
    _Config2[1] = 0b00000110;

    // Set Configuration bits [Config1]
    if (MAX17055_Ber) bitSet(_Config1[0], 0);
    if (MAX17055_Bei) bitSet(_Config1[0], 1);
    if (MAX17055_Aen) bitSet(_Config1[0], 2);
    if (MAX17055_FTHRM) bitSet(_Config1[0], 3);
    if (MAX17055_ETHRM) bitSet(_Config1[0], 4);
    if (MAX17055_COMMSH) bitSet(_Config1[0], 6);
    if (MAX17055_SHDN) bitSet(_Config1[0], 7);
    if (MAX17055_Tex) bitSet(_Config1[1], 0);
    if (MAX17055_Ten) bitSet(_Config1[1], 1);
    if (MAX17055_AINSH) bitSet(_Config1[1], 2);
    if (MAX17055_IS) bitSet(_Config1[1], 3);
    if (MAX17055_VS) bitSet(_Config1[1], 4);
    if (MAX17055_TS) bitSet(_Config1[1], 5);
    if (MAX17055_SS) bitSet(_Config1[1], 6);
    if (MAX17055_TSel) bitSet(_Config1[1], 7);

    // Set Configuration bits [Config2]
    if (MAX17055_CPMode) bitSet(_Config2[0], 1);
    if (MAX17055_LDMDL) bitSet(_Config2[0], 5);
    if (MAX17055_TAIrtEN) bitSet(_Config2[0], 6);
    if (MAX17055_dSOCen) bitSet(_Config2[0], 7);
    if (MAX17055_DPEn) bitSet(_Config2[1], 4);
    if (MAX17055_AtRateEn) bitSet(_Config2[1], 5);

    // Config1 Setting
    writeMultipleRegister(0x1D, _Config1, 2);
    writeMultipleRegister(0xBB, _Config2, 2);
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

    int32_t _Value = (_Signed_Raw * 15625) / (__MAX17055_Resistor__ * 100);

    // End Function
    return _Value;
}

int32_t MAX17055::averageCurrent(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x0B);

    // Convert to signed int16_t (two's complement)
    int32_t _Signed_Raw = static_cast<int16_t>(_Measurement_Raw);

    int32_t _Value = (_Signed_Raw * 15625) / (__MAX17055_Resistor__ * 100);

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

    // Calculate Data
    uint16_t _Value = _Measurement_Raw * 5 / 1000 / __MAX17055_Resistor__;

    // End Function
    return _Value;
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
    // Define Data Variable
    uint8_t _Status_Register[2] = {0x00, 0x00};

    // Read Status Register
    readMultipleRegister(0x00, _Status_Register, 2, false);

    // Control for Status
    if (_Status == MAX17055_POR)
        return bitRead(_Status_Register[0], 1);
    else if (_Status == MAX17055_IMin)
        return bitRead(_Status_Register[0], 2);
    else if (_Status == MAX17055_IMax)
        return bitRead(_Status_Register[0], 6);
    else if (_Status == MAX17055_VMin)
        return bitRead(_Status_Register[1], 0);
    else if (_Status == MAX17055_VMax)
        return bitRead(_Status_Register[1], 4);
    else if (_Status == MAX17055_TMin)
        return bitRead(_Status_Register[1], 1);
    else if (_Status == MAX17055_TMax)
        return bitRead(_Status_Register[1], 5);
    else if (_Status == MAX17055_SOC_Min)
        return bitRead(_Status_Register[1], 2);
    else if (_Status == MAX17055_SOC_Max)
        return bitRead(_Status_Register[1], 6);
    else if (_Status == MAX17055_SOC_Change)
        return bitRead(_Status_Register[0], 7);
    else if (_Status == MAX17055_Bat_Status)
        return bitRead(_Status_Register[0], 3);
    else if (_Status == MAX17055_Bat_Insert)
        return bitRead(_Status_Register[1], 3);
    else if (_Status == MAX17055_Bat_Remove)
        return bitRead(_Status_Register[1], 7);

    // End Function
    return false;
}

void MAX17055::statusClear(void) {
    // Define Data Variable
    const uint8_t _Status_Register[2] = {0x00, 0x00};

    // Write Status Register
    writeMultipleRegister(0x00, _Status_Register, 2);
}

uint16_t MAX17055::chargeTerminationCurrent(void) {
    // Get Data from IC
    uint16_t _Measurement_Raw = read_register(0x1E);

    // Calculate Data
    uint16_t Value = (((uint32_t)_Measurement_Raw * 1.5625) / __MAX17055_Resistor__);

    // End Function
    return Value;
}

} /* namespace max17055 */
}  // namespace battery