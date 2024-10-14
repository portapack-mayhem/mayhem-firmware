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

#include "standalone_app.hpp"
#include "i2cdevmanager.hpp"

namespace i2cdev {

class I2cDev_PPmod : public I2cDev {
   public:
    enum class Command : uint16_t {
        COMMAND_NONE = 0,

        // will respond with device_info
        COMMAND_INFO = 0x18F0,

        // will respond with info of application
        COMMAND_APP_INFO = 0xA90B,

        // will respond with application data
        COMMAND_APP_TRANSFER = 0x4183,
    };

    typedef struct {
        uint32_t api_version;
        uint32_t module_version;
        char module_name[20];
        uint32_t application_count;
    } device_info;

    typedef struct {
        uint32_t header_version;
        uint8_t app_name[16];
        uint8_t bitmap_data[32];
        uint32_t icon_color;
        app_location_t menu_location;
        uint32_t binary_size;
    } standalone_app_info;

    bool init(uint8_t addr_) override;
    void update() override;

    std::optional<device_info> readDeviceInfo();
    std::optional<standalone_app_info> getStandaloneAppInfo(uint32_t index);
    std::vector<uint8_t> downloadStandaloneApp(uint32_t index, size_t offset);
};

} /* namespace i2cdev */
