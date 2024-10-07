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

#include <cstdint>
#include <array>
#include <string>
#include <optional>
#include "battery.hpp"
#include "i2cdevmanager.hpp"

namespace i2cdev {

class I2cDev_PPmod : public I2cDev {
   public:
    typedef struct {
        uint32_t api_version;
        uint32_t module_version;
        char module_name[20];
    } device_info;

    typedef struct {
        uint32_t api_version;
        char name[32];
        uint32_t binary_size;
    } standalone_app_info;

    bool init(uint8_t addr_) override;
    void update() override;

    std::optional<device_info> readDeviceInfo();
    std::vector<standalone_app_info> getStandaloneApps();
    std::vector<uint8_t> downloadStandaloneApp(size_t offset, size_t length);
};

} /* namespace i2cdev */
