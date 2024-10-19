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

#include "i2cdev_ppmod.hpp"
#include "portapack.hpp"
#include <optional>

namespace i2cdev {

bool I2cDev_PPmod::init(uint8_t addr_) {
    if (addr_ != I2CDEV_PPMOD_ADDR_1) return false;
    addr = addr_;
    model = I2CDECMDL_PPMOD;
    query_interval = 10;

    return true;
}

void I2cDev_PPmod::update() {
    auto mask = get_features_mask();
    if (mask & (uint64_t)SupportedFeatures::FEAT_GPS) {
        auto data = get_gps_data();
        if (data.has_value()) {
            GPSPosDataMessage msg{data.value().latitude, data.value().longitude, (int32_t)data.value().altitude, (int32_t)data.value().speed, data.value().sats_in_use};
            EventDispatcher::send_message(msg);
        }
    }
    if (mask & (uint64_t)SupportedFeatures::FEAT_ORIENTATION) {
        auto data = get_orientation_data();
        if (data.has_value()) {
            OrientationDataMessage msg{(uint16_t)data.value().angle, (int16_t)data.value().tilt};
            EventDispatcher::send_message(msg);
        }
    }
    if (mask & (uint64_t)SupportedFeatures::FEAT_ENVIRONMENT) {
        auto data = get_environment_data();
        if (data.has_value()) {
            EnvironmentDataMessage msg{data.value().temperature, data.value().humidity, data.value().pressure};
            EventDispatcher::send_message(msg);
        }
    }
    if (mask & (uint64_t)SupportedFeatures::FEAT_LIGHT) {
        auto data = get_light_data();
        if (data.has_value()) {
            LightDataMessage msg{data.value()};
            EventDispatcher::send_message(msg);
        }
    }
}

std::optional<orientation_t> I2cDev_PPmod::get_orientation_data() {
    Command cmd = Command::COMMAND_GETFEAT_DATA_ORIENTATION;
    orientation_t data;
    bool success = i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&data, sizeof(orientation_t));
    if (success == false) {
        return std::nullopt;
    }
    return data;
}

std::optional<gpssmall_t> I2cDev_PPmod::get_gps_data() {
    Command cmd = Command::COMMAND_GETFEAT_DATA_GPS;
    gpssmall_t data;
    bool success = i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&data, sizeof(gpssmall_t));
    if (success == false) {
        return std::nullopt;
    }
    return data;
}

std::optional<environment_t> I2cDev_PPmod::get_environment_data() {
    Command cmd = Command::COMMAND_GETFEAT_DATA_ENVIRONMENT;
    environment_t data;
    bool success = i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&data, sizeof(environment_t));
    if (success == false) {
        return std::nullopt;
    }
    return data;
}

std::optional<uint16_t> I2cDev_PPmod::get_light_data() {
    Command cmd = Command::COMMAND_GETFEAT_DATA_LIGHT;
    uint16_t data;
    bool success = i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&data, sizeof(uint16_t));
    if (success == false) {
        return std::nullopt;
    }
    return data;
}

uint64_t I2cDev_PPmod::get_features_mask() {
    uint64_t mask = 0;
    Command cmd = Command::COMMAND_GETFEATURE_MASK;
    bool success = i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&mask, sizeof(mask));
    if (success == false) {
        return 0;
    }
    // sanity check
    if (mask == UINT64_MAX) {
        return 0;
    }
    return mask;
}

std::optional<I2cDev_PPmod::device_info> I2cDev_PPmod::readDeviceInfo() {
    Command cmd = Command::COMMAND_INFO;
    I2cDev_PPmod::device_info info;

    bool success = i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&info, sizeof(I2cDev_PPmod::device_info));
    if (success == false) {
        return std::nullopt;
    }
    // sanity check
    if (info.application_count > 1000) {
        return std::nullopt;
    }
    return info;
}

std::optional<I2cDev_PPmod::standalone_app_info> I2cDev_PPmod::getStandaloneAppInfo(uint32_t index) {
    Command cmd = Command::COMMAND_APP_INFO;
    uint32_t data = (uint32_t)cmd + (index << 16);
    I2cDev_PPmod::standalone_app_info info;

    bool success = i2c_read((uint8_t*)&data, 4, (uint8_t*)&info, sizeof(I2cDev_PPmod::standalone_app_info));
    if (success == false) {
        return std::nullopt;
    }
    // sanity check
    if (info.binary_size == UINT32_MAX) {
        return std::nullopt;
    }

    return info;
}

constexpr size_t transfer_block_size = 128;

std::vector<uint8_t> I2cDev_PPmod::downloadStandaloneApp(uint32_t index, size_t offset) {
    if (offset % transfer_block_size != 0) {
        return {};
    }

    uint16_t data[3] = {
        static_cast<uint16_t>(Command::COMMAND_APP_TRANSFER),
        static_cast<uint16_t>(index & 0xFFFF),                          // keep index in 16 bits range
        static_cast<uint16_t>((offset / transfer_block_size) & 0xFFFF)  // keep (offset / transfer_block_size) in 16 bits range
    };

    /*
    // TODO: check if there was an out of range, manage error
    if (index > std::numeric_limits<uint16_t>::max()) {
        // manage error if index is bigger than a 16 bits value
    }
    // TODO: check if there was an out of range, manage error
    if (offset / transfer_block_size > std::numeric_limits<uint16_t>::max()) {
        // manage error if (offset / transfer_block_size ) is bigger than a 16 bits value
    }
    */
    std::vector<uint8_t> ret(transfer_block_size);
    bool success = i2c_read((uint8_t*)&data, sizeof(data), (uint8_t*)ret.data(), transfer_block_size);
    if (success == false) {
        return {};
    }

    return ret;
}

}  // namespace i2cdev
