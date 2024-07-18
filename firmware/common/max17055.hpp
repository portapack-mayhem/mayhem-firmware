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

#ifndef __MAX17055_H__
#define __MAX17055_H__

#include <cstdint>
#include <array>
#include <string>

#include "i2c_pp.hpp"

#define MAX17055_POR 0
#define MAX17055_IMin 1
#define MAX17055_IMax 2
#define MAX17055_VMin 3
#define MAX17055_VMax 4
#define MAX17055_TMin 5
#define MAX17055_TMax 6
#define MAX17055_SOC_Min 7
#define MAX17055_SOC_Max 8
#define MAX17055_SOC_Change 9
#define MAX17055_Bat_Status 10
#define MAX17055_Bat_Insert 11
#define MAX17055_Bat_Remove 12

// Design Parameters
//----------------------------------------------

// Define Battery Capacity
#ifndef __MAX17055_Design_Capacity__
#define __MAX17055_Design_Capacity__ 2500  // Battery Capacity
#endif

// Define Gauge Resistor
#ifndef __MAX17055_Resistor__
#define __MAX17055_Resistor__ 0.01  // Shunt Resistor
#endif

// Define Minimum Voltage
#ifndef __MAX17055_Min_Voltage__
#define __MAX17055_Min_Voltage__ 3.0  // Minimum Voltage
#endif

// Define Maximum Voltage
#ifndef __MAX17055_Max_Voltage__
#define __MAX17055_Max_Voltage__ 4.17  // Maximum Voltage
#endif

// Define Empty Voltage
#ifndef __MAX17055_Empty_Voltage__
#define __MAX17055_Empty_Voltage__ 3.0  // Empty Voltage
#endif

// Define Recovery Voltage
#ifndef __MAX17055_Recovery_Voltage__
#define __MAX17055_Recovery_Voltage__ 3.1  // Recovery Voltage
#endif

// Define Maximum Current
#ifndef __MAX17055_Max_Current__
#define __MAX17055_Max_Current__ 3.0  // Maximum Current
#endif

// Define Termination Current
#ifndef __MAX17055_Termination_Current__
#define __MAX17055_Termination_Current__ 0.1  // Termination Current
#endif

// Define Minimum Temperature
#ifndef __MAX17055_Min_Temperature__
#define __MAX17055_Min_Temperature__ -20  // Minimum Temperature
#endif

// Define Maximum Temperature
#ifndef __MAX17055_Max_Temperature__
#define __MAX17055_Max_Temperature__ 60  // Maximum Temperature
#endif

// Define Battery Model
#ifndef __MAX17055_Battery_Model__
#define __MAX17055_Battery_Model__ 0  // Battery Model
#endif

// Define Limits
//----------------------------------------------

// Define Minimum SOC
#ifndef __MAX17055_Min_SOC__
#define __MAX17055_Min_SOC__ 0  // Minimum SOC
#endif

// Define Maximum SOC
#ifndef __MAX17055_Max_SOC__
#define __MAX17055_Max_SOC__ 100  // Maximum SOC
#endif

// Config1 (0x1D) Configuration
//----------------------------------------------

// Enable alert on battery removal when the IC is mounted host side.
// When Ber = 1, a battery-removal condition, as detected by the AIN pin voltage, triggers an alert.
#ifndef MAX17055_Ber
#define MAX17055_Ber 0
#endif

// Enable alert on battery insertion when the IC is mounted host side.
// When Bei = 1, a battery-insertion condition, as detected by the AIN pin voltage, triggers an alert.
#ifndef MAX17055_Bei
#define MAX17055_Bei 0
#endif

// When Aen = 1, violation of any of the alert threshold register values
// by temperature, voltage, or SOC triggers an alert. This bit
// affects the ALRT pin operation only. The Smx, Smn, Tmx,
// Tmn, Vmx, Vmn, Imx, and Imn bits of the Status register
// (00h) are not disabled.
#ifndef MAX17055_Aen
#define MAX17055_Aen 1
#endif

// This allows the host to control the bias of the thermistor switch or
// enable fast detection of battery removal. Set FTHRM = 1
// to always enable the thermistor bias switch. With a standard 10kΩ thermistor,
// this adds an  additional ~200μA to the current drain of the circuit.
#ifndef MAX17055_FTHRM
#define MAX17055_FTHRM 0
#endif

// Set to logic 1 to enable the automatic THRM output bias and AIN measurement.
#ifndef MAX17055_ETHRM
#define MAX17055_ETHRM 0
#endif

// Set to logic 1 to force the device to enter shutdown mode if both
// SDA and SCL are held low for more than timeout of the
// ShdnTimer register.
#ifndef MAX17055_COMMSH
#define MAX17055_COMMSH 0
#endif

// Write this bit to logic 1 to force a shutdown of the device after timeout of the ShdnTimer register (default 45s delay).
#ifndef MAX17055_SHDN
#define MAX17055_SHDN 0
#endif

// When set to 1, the fuel gauge requires external temperature measurements to be
// written from the host. When set to 0, the IC's own measurements as used as selected by Config.TSEL.
#ifndef MAX17055_Tex
#define MAX17055_Tex 0
#endif

// Set to 1 and set ETHRM or FTHRM to 1 to enable temperature measurements selected by Config.TSel.
#ifndef MAX17055_Ten
#define MAX17055_Ten 1
#endif

// Set to 1 to enable device shutdown when the IC is mounted host side and the battery is removed.
#ifndef MAX17055_AINSH
#define MAX17055_AINSH 0
#endif

// When IS = 1, current alerts can only be cleared through software. When IS = 0,
// current alerts are cleared automatically when the thresholdis no longer exceeded.
#ifndef MAX17055_IS
#define MAX17055_IS 0
#endif

// When VS = 1, voltage alerts can only be cleared through software. When VS = 0,
// voltage alerts are cleared automatically when the threshold is no longer exceeded.
#ifndef MAX17055_VS
#define MAX17055_VS 0
#endif

// When TS = 1, temperature alerts can only be cleared through software. WhenTS = 0,
// temperature alerts are cleared automatically whenthe threshold is no longer exceeded.
#ifndef MAX17055_TS
#define MAX17055_TS 0
#endif

// When SS = 1, SOC alerts can only be cleared through software. When SS = 0,
// SOC alerts are cleared automatically when the threshold is no longer exceeded.
#ifndef MAX17055_SS
#define MAX17055_SS 0
#endif

// 0 to use internal die temperature. 1 to use temperature information from thermistor.
// ETHRM bit should be set to 1 when TSel is 1.
#ifndef MAX17055_TSel
#define MAX17055_TSel 0
#endif

// Config1 (0xBB) Configuration
//----------------------------------------------

// Set to 1 to enable constant-power mode. If it is set to 0,
// AtRate/AvgCurrent is used for (At)TTE/(At) QResidual/(At)AvSOC/(At)AvCap.
#ifndef MAX17055_CPMode
#define MAX17055_CPMode 0
#endif

// Host sets this bit to 1 in order to initiate firmware to finish processing a newly loaded model.
// Firmware clears this bit to zero to indicate that model loading is finished.
#ifndef MAX17055_LDMDL
#define MAX17055_LDMDL 0
#endif

// Set this bit to 1 to enable temperature based alerts.
// Write this bit to 0 to disable temperature alerts. This bit is set to 1 at power-up.
#ifndef MAX17055_TAIrtEN
#define MAX17055_TAIrtEN 1
#endif

// Set this bit to 1 to enable alert output with the Status.dSOCi bit function.
// Write this bit to 0 to disable alert output with the Status. dSOCi bit. This bit is set to 0 at power-up.
#ifndef MAX17055_dSOCen
#define MAX17055_dSOCen 0
#endif

// When this bit is set to 0, Dynamic Power calculations are disabled and registers
// MaxPeakPower/SusPeakPower/MPPCurrent/ SPPCurrent can be used as general purpose memory.
#ifndef MAX17055_DPEn
#define MAX17055_DPEn 0
#endif

// When this bit is set to 0, AtRate calculations are disabled and registers
// AtQResidual/AtTTE/AtAvSOC/AtAvCap can be used as general purpose memory.
#ifndef MAX17055_AtRateEn
#define MAX17055_AtRateEn 0
#endif

namespace battery {
namespace max17055 {

using address_t = uint8_t;

class MAX17055 {
   public:
    constexpr MAX17055(I2C& bus, const I2C::address_t bus_address)
        : bus(bus), bus_address(bus_address), detected_(false) {}

    void init();
    bool detect();
    bool isDetected() const { return detected_; }

    uint16_t readVoltage();
    uint8_t readPercentage();
    void getBatteryInfo(uint8_t& valid_mask, uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current);

    uint16_t instantVoltage(void);
    uint16_t averageVoltage(void);
    uint16_t emptyVoltage(void);
    uint16_t recoveryVoltage(void);
    int32_t instantCurrent(void);
    int32_t averageCurrent(void);
    uint16_t stateOfCharge(void);
    uint16_t averageStateOfCharge(void);
    uint16_t instantCapacity(void);
    uint16_t designCapacity(void);
    uint16_t fullCapacity(void);
    uint16_t icTemperature(void);
    uint16_t timeToEmpty(void);
    uint16_t timeToFull(void);
    uint16_t batteryAge(void);
    uint16_t chargeCycle(void);
    bool statusControl(const uint8_t _Status);
    void statusClear(void);
    uint16_t chargeTerminationCurrent(void);
    uint16_t read_register(const uint8_t reg);
    bool write_register(const uint8_t reg, const uint16_t value);

   private:
    I2C& bus;
    const I2C::address_t bus_address;
    bool detected_ = false;

    bool readRegister(uint8_t reg, uint16_t& value);
    bool readMultipleRegister(uint8_t reg, uint8_t* data, uint8_t length, bool endTransmission);
    bool writeMultipleRegister(uint8_t reg, const uint8_t* data, uint8_t length);

    bool setEmptyVoltage(uint16_t _Empty_Voltage);
    bool setRecoveryVoltage(uint16_t _Recovery_Voltage);
    bool setMinVoltage(uint16_t _Minimum_Voltage);
    bool setMaxVoltage(uint16_t _Maximum_Voltage);
    bool setMaxCurrent(uint16_t _Maximum_Current);
    bool setChargeTerminationCurrent(uint16_t _Charge_Termination_Current);
    bool setDesignCapacity(const uint16_t _Capacity);
    bool setFullCapRep(const uint16_t _Capacity);
    bool setFullCapNom(const uint16_t _Capacity);
    bool setRepCap(const uint16_t _Capacity);
    bool setMinSOC(uint8_t _Minimum_SOC);
    bool setMaxSOC(uint8_t _Maximum_SOC);
    bool setMinTemperature(uint8_t _Minimum_Temperature);
    bool setMaxTemperature(uint8_t _Maximum_Temperature);
    bool setModelCfg(const uint8_t _Model_ID);
    bool setHibCFG(const uint16_t _Config);
    void config(void);
};

} /* namespace max17055 */
}  // namespace battery
#endif /* __MAX17055_H__ */