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

namespace battery {

#define BATTERY_MIN_VOLTAGE 3000.0
#define BATTERY_MAX_VOLTAGE 4170.0
#define BATTERY_DESIGN_CAP 2500
#define BATTERY_DESIGN_CAP_PRF 3000

// this will just hold the config and a calculation. also some defines above for the battery management. could be movet to the classes, but it is a bit more readeable i think.
class BatteryManagement {
   public:
    enum BatteryValidMask {
        BATT_VALID_NONE = 0,
        BATT_VALID_VOLTAGE = 1,
        BATT_VALID_CURRENT = 2,
        BATT_VALID_PERCENT = 4,
        BATT_VALID_CYCLES = 8,
        BATT_VALID_TTEF = 16,
    };
    static bool isDetected();
    static void set_calc_override(bool override);
    static uint8_t calc_percent_voltage(uint16_t);  // calculates battery percentage from the voltage
    static bool calcOverride;                       // if set to true, it'll override the battery percent calculation based on current voltage.
    static void getBatteryInfo(uint8_t& valid_mask, uint8_t& percent, uint16_t& voltage, int32_t& current);
    static uint16_t get_cycles();
    static float get_tte();
    static float get_ttf();

   private:
};
};  // namespace battery
#endif