/*
 * Copyright (C) 2024 Bernd Herzog
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

#pragma once

#include "i2c_pp.hpp"

#include <optional>
#include <vector>

class ExternalModule {
   public:
    typedef struct {
        uint32_t api_version;
        uint32_t module_version;
        char module_name[20];
    } device_info;

    typedef struct {
        uint32_t api_version;
        char name[32];
        uint32_t module_version;
    } standalone_app_info;

    constexpr ExternalModule(I2C& bus, I2C::address_t address)
        : bus_(bus), address_(address) {}

    std::optional<device_info> get_device_info() const;
    std::vector<standalone_app_info> get_standalone_apps() const;
    std::vector<uint8_t> download_standalone_app() const;

   private:
    I2C& bus_;
    I2C::address_t address_;
};
