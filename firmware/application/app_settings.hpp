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
#include <string_view>
#include <utility>
#include <variant>

#include "file.hpp"
#include "max283x.hpp"
#include "string_format.hpp"

namespace app_settings {

enum class ResultCode : uint8_t {
    Ok,                // settings found
    LoadFailed,        // settings (file) not found
    SaveFailed,        // unable to save settings
    SettingsDisabled,  // load/save disabled in settings
};

enum class Mode : uint8_t {
    RX = 0x01,
    TX = 0x02,
    RX_TX = 0x03,  // Both TX/RX
};

enum class Options {
    None = 0x0000,

    /* Don't use target frequency from app settings. */
    UseGlobalTargetFrequency = 0x0001,
};

// TODO: separate types for TX/RX or union?
/* NB: See RX/TX model headers for default values. */
struct AppSettings {
    Mode mode = Mode::RX;
    Options options = Options::None;
    uint32_t baseband_bandwidth = max283x::filter::bandwidth_minimum;
    uint32_t sampling_rate = 3072000;  // Good for 48k audio.
    uint8_t lna = 32;
    uint8_t vga = 32;
    uint8_t rx_amp = 0;
    uint8_t tx_amp = 0;
    uint8_t tx_gain = 35;
    uint32_t channel_bandwidth = 1;
    uint32_t rx_frequency;
    uint32_t tx_frequency;
    uint32_t step = 25000;
    uint8_t modulation = 1;  // NFM
    uint8_t am_config_index = 0;
    uint8_t nbfm_config_index = 0;
    uint8_t wfm_config_index = 0;
    uint8_t squelch = 80;

    uint8_t volume;
};

using SettingBinding = std::pair<std::string_view, std::variant<uint32_t*, uint8_t*, bool*>>;
using SettingsList = std::vector<SettingBinding>;

ResultCode load_settings(
    const std::string& app_name,
    AppSettings& settings,
    const SettingsList& additional_settings);
ResultCode save_settings(
    const std::string& app_name,
    AppSettings& settings,
    const SettingsList& additional_settings);

/* Copies common values to the receiver/transmitter models. */
void copy_to_radio_model(const AppSettings& settings);

/* Copies common values from the receiver/transmitter models. */
void copy_from_radio_model(AppSettings& settings);

/* RAII wrapper for automatically loading and saving settings for an app.
 * NB: This should be added to a class before any LNA/VGA controls so that
 * the receiver/transmitter models are set before the control ctors run. */
class SettingsManager {
   public:
    SettingsManager(std::string app_name, Mode mode);
    SettingsManager(
        std::string app_name,
        Mode mode,
        SettingsList additional_settings);
    SettingsManager(
        std::string app_name,
        Mode mode,
        Options options);
    SettingsManager(
        std::string app_name,
        Mode mode,
        Options options,
        SettingsList additional_settings);
    ~SettingsManager();

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = delete;
    SettingsManager& operator=(SettingsManager&&) = delete;

    /* True if settings were successfully loaded from file. */
    bool loaded() const { return loaded_; }
    Mode mode() const { return settings_.mode; }

    AppSettings& raw() { return settings_; }

   private:
    std::string app_name_;
    AppSettings settings_;
    SettingsList additional_settings_;
    bool loaded_;
};

}  // namespace app_settings

ENABLE_FLAGS_OPERATORS(app_settings::Mode);
ENABLE_FLAGS_OPERATORS(app_settings::Options);

#endif /*__APP_SETTINGS_H__*/
