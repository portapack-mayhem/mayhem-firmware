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

class BatteryManagement {
   public:
    enum BatteryModules {
        BATT_NONE = 0,
        BATT_ADS1110 = 1,
        BATT_MAX17055 = 2,
        BATT_EMULATOR = 254
    };
    static void init();
    static bool isDetected() { return detected_ != BATT_NONE; }
    static BatteryModules detectedModule() { return detected_; }
    static bool getBatteryInfo(uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current);
    static uint16_t getVoltage();
    static uint8_t getPercent();
    static uint16_t read_register(const uint8_t reg);
    static bool write_register(const uint8_t reg, const uint16_t value);

   private:
    static void create_thread();
    static msg_t timer_fn(void* arg);
    static Thread* thread;
    static BatteryModules detected_;  // if there is any batt management system
};
};  // namespace battery
#endif