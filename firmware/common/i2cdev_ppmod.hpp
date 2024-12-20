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

class I2cDev_PPmod : public I2cDev {
   public:
    enum class Command : uint16_t {
        COMMAND_NONE = 0,

        // will respond with device_info
        COMMAND_INFO = 1,

        // will respond with info of application
        COMMAND_APP_INFO,

        // will respond with application data
        COMMAND_APP_TRANSFER,

        // Feature specific commands
        COMMAND_GETFEATURE_MASK,
        // Feature data getter commands
        COMMAND_GETFEAT_DATA_GPS,
        COMMAND_GETFEAT_DATA_ORIENTATION,
        COMMAND_GETFEAT_DATA_ENVIRONMENT,
        COMMAND_GETFEAT_DATA_LIGHT,
        // Shell specific communication
        COMMAND_SHELL_PPTOMOD_DATA,       // pp shell to esp. size not defined
        COMMAND_SHELL_MODTOPP_DATA_SIZE,  // how many bytes the esp has to send to pp's shell
        COMMAND_SHELL_MODTOPP_DATA,       // the actual bytes sent by esp. 1st byte's 1st bit is the "hasmore" flag, the remaining 7 bits are the size of the data. exactly 64 byte follows.

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
    uint16_t get_shell_buffer_bytes();
    bool get_shell_get_buffer_data(uint8_t* buff, size_t len);

   private:
    uint8_t self_timer = 0;
    uint64_t mask = 0;  // feauture mask, that indicates what the device can do. this will determinate what we will query from the device
};

} /* namespace i2cdev */

#endif
