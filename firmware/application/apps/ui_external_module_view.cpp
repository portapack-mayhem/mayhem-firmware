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

#include "ui_external_module_view.hpp"
#include "portapack.hpp"
#include "ui_standalone_view.hpp"
#include "theme.hpp"

#include "i2cdevmanager.hpp"
#include "i2cdev_ppmod.hpp"

#include <optional>

namespace ui {

void ExternalModuleView::focus() {
    menu_apps.focus();
}

void ExternalModuleView::on_tick_second() {
    i2cdev::I2CDevManager::manual_scan();

    auto dev = (i2cdev::I2cDev_PPmod*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);

    if (!dev) {
        text_header.set("No module connected");
        text_name.set("");
        text_version.set("");
        text_number_apps.set("");
        if (shown_count_ != -1) {
            menu_apps.clear();
            shown_count_ = -1;
        }
        return;
    }

    auto device_info = dev->readDeviceInfo();

    if (device_info.has_value() == false) {
        text_header.set("No module connected");
        text_name.set("");
        text_version.set("");
        text_number_apps.set("");
        if (shown_count_ != -1) {
            menu_apps.clear();
            shown_count_ = -1;
        }
        return;
    }

    text_header.set("Module found");

    std::string btnText = (std::string) "Module: " + device_info->module_name;
    text_name.set(btnText);
    text_version.set("Version: " + std::to_string(device_info->module_version));
    text_number_apps.set("No# Apps: " + std::to_string(device_info->application_count));

    // Only skip the rebuild when the app count is unchanged AND the current
    // list is complete. If some getStandaloneAppInfo() calls failed transiently
    // (e.g. an I2C read glitch) the menu holds fewer items than reported, so we
    // rebuild to let the list self-heal on a later tick.
    if ((int32_t)device_info->application_count == shown_count_ &&
        menu_apps.item_count() == (size_t)device_info->application_count) {
        return;
    }

    menu_apps.clear();

    for (uint32_t i = 0; i < device_info->application_count; i++) {
        auto appInfo = dev->getStandaloneAppInfo(i);
        if (appInfo.has_value() == false) {
            continue;
        }

        std::string itemText = (std::string) "App " + std::to_string(i + 1) + ": " + (const char*)appInfo->app_name;

        switch (appInfo->menu_location) {
            case app_location_t::UTILITIES:
                itemText += " (Utilities)";
                break;
            case app_location_t::RX:
                itemText += " (RX)";
                break;
            case app_location_t::TX:
                itemText += " (TX)";
                break;
            case app_location_t::TRX:
                itemText += " (TRX)";
                break;
            case app_location_t::SETTINGS:
                itemText += " (Settings)";
                break;
            case app_location_t::DEBUG:
                itemText += " (Debug)";
                break;
            case app_location_t::HOME:
                itemText += " (Home)";
                break;
            case app_location_t::GAMES:
                itemText += " (Games)";
                break;
        }

        menu_apps.add_item({itemText, ui::Theme::getInstance()->fg_light->foreground, nullptr, [](KeyEvent) {}});
    }

    shown_count_ = (int32_t)device_info->application_count;
}

}  // namespace ui
