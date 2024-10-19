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

#ifndef __I2CDEV_PPMOD_H
#define __I2CDEV_PPMOD_H

#include <cstdint>
#include <array>
#include <string>
#include <optional>

#include "standalone_app.hpp"
#include "i2cdevmanager.hpp"

#include "i2cdev_ppmod_helper.hpp"

namespace i2cdev {

#define USER_COMMANDS_START 0x7F01

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

        // UART specific commands
        COMMAND_UART_REQUESTDATA_SHORT = USER_COMMANDS_START,
        COMMAND_UART_REQUESTDATA_LONG,
        COMMAND_UART_BAUDRATE_INC,
        COMMAND_UART_BAUDRATE_DEC,
        COMMAND_UART_BAUDRATE_GET,
        // Sensor specific commands
        COMMAND_GETFEATURE_MASK,
        COMMAND_GETFEAT_DATA_GPS,
        COMMAND_GETFEAT_DATA_ORIENTATION,
        COMMAND_GETFEAT_DATA_ENVIRONMENT,
        COMMAND_GETFEAT_DATA_LIGHT,

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

    std::optional<standalone_app_info> getStandaloneAppInfo(uint32_t index);
    std::vector<uint8_t> downloadStandaloneApp(uint32_t index, size_t offset);
    uint64_t get_features_mask();
    std::optional<device_info> readDeviceInfo();
    std::optional<gpssmall_t> get_gps_data();
    std::optional<orientation_t> get_orientation_data();
    std::optional<environment_t> get_environment_data();
    std::optional<uint16_t> get_light_data();
};

} /* namespace i2cdev */

#endif
