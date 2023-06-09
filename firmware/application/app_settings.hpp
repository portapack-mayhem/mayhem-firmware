/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2022 Arjan Onwezen
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

#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "file.hpp"
#include "string_format.hpp"

namespace app_settings {

enum class AppSettingsResult : uint8_t {
    Ok,                // settings found
    LoadFailed,        // settings (file) not found
    SaveFailed,        // unable to save settings
    SettingsDisabled,  // load/save disabled in settings
};

struct AppSettings {
    // The following are accessed via receiver/transmitter models.
    uint32_t baseband_bandwidth;
    uint32_t sampling_rate;
    uint32_t channel_bandwidth;
    uint8_t lna;
    uint8_t vga;
    uint8_t rx_amp;
    uint8_t tx_amp;
    uint8_t tx_gain;

    uint32_t rx_frequency;
    uint32_t tx_frequency;
    uint32_t step;
    uint8_t modulation;
    uint8_t am_config_index;
    uint8_t nbfm_config_index;
    uint8_t wfm_config_index;
    uint8_t squelch;
};

AppSettingsResult load_settings(const std::string& app_name, AppSettings& settings);
AppSettingsResult save_settings(const std::string& app_name, AppSettings& settings);

/* Copies common values to the receiver/transmitter models. */
void copy_to_radio_model(const AppSettings& settings);

/* Copies common values from the receiver/transmitter models. */
void copy_from_radio_model(AppSettings& settings);

/* RAII wrapper for automatically loading and saving settings for an app.
 * NB: This should be added to a class before any LNA/VGA controls so that
 * the receiver/transmitter models are set before the control ctors run. */
class AutoAppSettings {
   public:
    AutoAppSettings(std::string application);
    ~AutoAppSettings();

    // TODO: No move/copy.

    AppSettings& settings() { return settings_; }

   private:
    std::string app_name_;
    AppSettings settings_;
};

}  // namespace app_settings

#endif /*__APP_SETTINGS_H__*/
