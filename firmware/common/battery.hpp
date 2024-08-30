/*
 * Copyright (C) 2024 HTotoo.
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

#ifndef __BATTERY_H__
#define __BATTERY_H__

#include <cstdint>
#include "ch.h"

namespace battery {

#define BATTERY_MIN_VOLTAGE 3000.0
#define BATTERY_MAX_VOLTAGE 4170.0
#define BATTERY_DESIGN_CAP 2500

class BatteryManagement {
   public:
    enum BatteryModules {
        BATT_NONE = 0,
        BATT_ADS1110 = 1,
        BATT_MAX17055 = 2,
        BATT_EMULATOR = 254
    };
    enum BatteryValidMask {
        BATT_VALID_NONE = 0,
        BATT_VALID_VOLTAGE = 1,
        BATT_VALID_CURRENT = 2,
        BATT_VALID_PERCENT = 4,
    };
    static void init(bool override = false);
    static void detect();
    static bool isDetected() { return detected_ != BATT_NONE; }
    static BatteryModules detectedModule() { return detected_; }
    static void getBatteryInfo(uint8_t& valid_mask, uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current);
    static uint16_t getVoltage();
    static uint8_t getPercent();
    static uint16_t read_register(const uint8_t reg);
    static bool write_register(const uint8_t reg, const uint16_t value);
    static void set_calc_override(bool override);
    static uint8_t calc_percent_voltage(uint16_t);  // calculates battery percentage from the voltage
    static bool reset_learned();                    // resets the ic's learned parameters

   private:
    static void create_thread();
    static msg_t timer_fn(void* arg);
    static Thread* thread;
    static BatteryModules detected_;  // if there is any batt management system
    static bool calcOverride;         // if set to true, it'll override the battery percent calculation based on current voltage.
};
};  // namespace battery
#endif