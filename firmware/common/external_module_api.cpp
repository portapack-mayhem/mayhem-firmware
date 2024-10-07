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

#include "external_module_api.hpp"
#include "portapack.hpp"

std::optional<ExternalModule::device_info> ExternalModule::get_device_info() const {
    // TODO: enable when implemented
    // if (bus_.probe(address_, 100) == false) {
    //     return std::nullopt;
    // }

    uint16_t cmd = 0x1234;

    bool success = bus_.transmit(address_, (uint8_t*)&cmd, 2, 1000);
    // return std::nullopt;
    if (success == false) {
        return std::nullopt;
    }

    chThdSleepMilliseconds(10);

    // bool rec = bus_.transfer(0x51, (uint8_t*)&addr, 1, (uint8_t*)&info, sizeof(ExternalModule::device_info), 1000);

    // uint8_t addr = 0x00;
    // bus_.transmit(0x50, (uint8_t*)&addr, 1, 1000);

    ExternalModule::device_info info;
    bool rec = bus_.receive(address_, (uint8_t*)&info, sizeof(ExternalModule::device_info) - 1, 1000);

    if (rec) {
        return info;
    } else {
        return std::nullopt;
    }
}

std::vector<ExternalModule::standalone_app_info> ExternalModule::get_standalone_apps() const {
    return {};
}

std::vector<uint8_t> ExternalModule::download_standalone_app() const {
    return {};
}
