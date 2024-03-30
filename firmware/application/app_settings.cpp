/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2022 Arjan Onwezen
 * Copyright (C) 2023 Kyle Reed
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

#include "app_settings.hpp"

#include "convert.hpp"
#include "file.hpp"
#include "file_reader.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "utility.hpp"
#include "file_path.hpp"

#include <algorithm>
#include <cstring>
#include <string_view>

namespace fs = std::filesystem;
using namespace portapack;

namespace {
fs::path get_settings_path(const std::string& app_name) {
    return settings_dir / app_name + u".ini";
}
}  // namespace

void BoundSetting::parse(std::string_view value) {
    switch (type_) {
        case SettingType::I64:
            parse_int(value, as<int64_t>());
            break;
        case SettingType::I32:
            parse_int(value, as<int32_t>());
            break;
        case SettingType::U32:
            parse_int(value, as<uint32_t>());
            break;
        case SettingType::U8:
            parse_int(value, as<uint8_t>());
            break;
        case SettingType::String:
            as<std::string>() = trim(value);
            break;
        case SettingType::Bool: {
            int parsed = 0;
            parse_int(value, parsed);
            as<bool>() = (parsed != 0);
            break;
        }
    };
}

void BoundSetting::write(File& file) const {
    // NB: Write directly without allocations. This happens on every
    // app exit when enabled so should be fast to keep the UX responsive.
    StringFormatBuffer buffer;
    size_t length = 0;

    file.write(name_.data(), name_.length());
    file.write("=", 1);

    switch (type_) {
        case SettingType::I64:
            file.write(to_string_dec_int(as<int64_t>(), buffer, length), length);
            break;
        case SettingType::I32:
            file.write(to_string_dec_int(as<int32_t>(), buffer, length), length);
            break;
        case SettingType::U32:
            file.write(to_string_dec_uint(as<uint32_t>(), buffer, length), length);
            break;
        case SettingType::U8:
            file.write(to_string_dec_uint(as<uint8_t>(), buffer, length), length);
            break;
        case SettingType::String: {
            const auto& str = as<std::string>();
            file.write(str.data(), str.length());
            break;
        }
        case SettingType::Bool:
            file.write(as<bool>() ? "1" : "0", 1);
            break;
    }

    file.write("\r\n", 2);
}

SettingsStore::SettingsStore(std::string_view store_name, SettingBindings bindings)
    : store_name_{store_name}, bindings_{bindings} {
    reload();
}

SettingsStore::~SettingsStore() {
    save();
}

void SettingsStore::reload() {
    load_settings(store_name_, bindings_);
}

void SettingsStore::save() const {
    save_settings(store_name_, bindings_);
}

bool load_settings(std::string_view store_name, SettingBindings& bindings) {
    File f;
    auto path = get_settings_path(std::string{store_name});

    auto error = f.open(path);
    if (error)
        return false;

    auto reader = FileLineReader(f);
    for (const auto& line : reader) {
        auto cols = split_string(line, '=');

        if (cols.size() != 2)
            continue;

        // Find a binding with the name.
        auto it = std::find_if(
            bindings.begin(), bindings.end(),
            [name = cols[0]](auto& bound_setting) {
                return name == bound_setting.name();
            });

        // If found, parse the value.
        if (it != bindings.end())
            it->parse(cols[1]);
    }

    return true;
}

bool save_settings(std::string_view store_name, const SettingBindings& bindings) {
    File f;
    auto path = get_settings_path(std::string{store_name});

    ensure_directory(settings_dir);
    auto error = f.create(path);
    if (error)
        return false;

    for (const auto& bound_setting : bindings)
        bound_setting.write(f);

    return true;
}

namespace app_settings {

void copy_to_radio_model(const AppSettings& settings) {
    // NB: Don't actually adjust the radio here or it may hang.
    // Specifically 'modulation' which requires a running baseband.

    if (flags_enabled(settings.mode, Mode::TX)) {
        persistent_memory::set_target_frequency(settings.tx_frequency);
        transmitter_model.configure_from_app_settings(settings);
    }

    if (flags_enabled(settings.mode, Mode::RX)) {
        persistent_memory::set_target_frequency(settings.rx_frequency);
        receiver_model.configure_from_app_settings(settings);
        receiver_model.set_configuration_without_update(
            static_cast<ReceiverModel::Mode>(settings.modulation),
            settings.step,
            settings.am_config_index,
            settings.nbfm_config_index,
            settings.wfm_config_index,
            settings.squelch);
    }

    receiver_model.set_frequency_step(settings.step);
    receiver_model.set_normalized_headphone_volume(settings.volume);
}

void copy_from_radio_model(AppSettings& settings) {
    if (flags_enabled(settings.mode, Mode::TX)) {
        settings.tx_frequency = transmitter_model.target_frequency();
        settings.baseband_bandwidth = transmitter_model.baseband_bandwidth();
        settings.sampling_rate = transmitter_model.sampling_rate();
        settings.tx_amp = transmitter_model.rf_amp();
        settings.tx_gain = transmitter_model.tx_gain();
        settings.channel_bandwidth = transmitter_model.channel_bandwidth();
    }

    if (flags_enabled(settings.mode, Mode::RX)) {
        settings.rx_frequency = receiver_model.target_frequency();
        settings.baseband_bandwidth = receiver_model.baseband_bandwidth();
        settings.sampling_rate = receiver_model.sampling_rate();
        settings.lna = receiver_model.lna();
        settings.vga = receiver_model.vga();
        settings.rx_amp = receiver_model.rf_amp();
        settings.squelch = receiver_model.squelch_level();

        settings.modulation = static_cast<uint8_t>(receiver_model.modulation());
        settings.am_config_index = receiver_model.am_configuration();
        settings.nbfm_config_index = receiver_model.nbfm_configuration();
        settings.wfm_config_index = receiver_model.wfm_configuration();
    }

    settings.step = receiver_model.frequency_step();
    settings.volume = receiver_model.normalized_headphone_volume();
}

/* SettingsManager *************************************************/
SettingsManager::SettingsManager(std::string_view app_name, Mode mode, Options options)
    : SettingsManager{app_name, mode, options, {}} {}

SettingsManager::SettingsManager(
    std::string_view app_name,
    Mode mode,
    SettingBindings additional_settings)
    : SettingsManager{app_name, mode, Options::None, std::move(additional_settings)} {}

SettingsManager::SettingsManager(
    std::string_view app_name,
    Mode mode,
    Options options,
    SettingBindings additional_settings)
    : app_name_{app_name},
      settings_{},
      bindings_{},
      loaded_{false} {
    settings_.mode = mode;
    settings_.options = options;

    // Pre-alloc enough for app settings and additional settings.
    additional_settings.reserve(COMMON_APP_SETTINGS_COUNT + additional_settings.size());
    bindings_ = std::move(additional_settings);

    // Settings should always be loaded because apps now rely
    // on being able to store UI settings, config, etc.

    // Transmitter model settings.
    if (flags_enabled(mode, Mode::TX)) {
        bindings_.emplace_back("tx_frequency"sv, &settings_.tx_frequency);
        bindings_.emplace_back("tx_amp"sv, &settings_.tx_amp);
        bindings_.emplace_back("tx_gain"sv, &settings_.tx_gain);
        bindings_.emplace_back("channel_bandwidth"sv, &settings_.channel_bandwidth);
    }

    // Receiver model settings.
    if (flags_enabled(mode, Mode::RX)) {
        bindings_.emplace_back("rx_frequency"sv, &settings_.rx_frequency);
        bindings_.emplace_back("lna"sv, &settings_.lna);
        bindings_.emplace_back("vga"sv, &settings_.vga);
        bindings_.emplace_back("rx_amp"sv, &settings_.rx_amp);
        bindings_.emplace_back("modulation"sv, &settings_.modulation);
        bindings_.emplace_back("am_config_index"sv, &settings_.am_config_index);
        bindings_.emplace_back("nbfm_config_index"sv, &settings_.nbfm_config_index);
        bindings_.emplace_back("wfm_config_index"sv, &settings_.wfm_config_index);
        bindings_.emplace_back("squelch"sv, &settings_.squelch);
    }

    // Common model settings.
    bindings_.emplace_back("baseband_bandwidth"sv, &settings_.baseband_bandwidth);
    bindings_.emplace_back("sampling_rate"sv, &settings_.sampling_rate);
    bindings_.emplace_back("step"sv, &settings_.step);
    bindings_.emplace_back("volume"sv, &settings_.volume);

    // RadioState should have initialized default radio parameters before this function;
    // copy them to settings_ before checking settings .ini file (in case the file doesn't exist
    // or doesn't include all parameters). Settings in the file can overwrite all, or a subset of parameters.
    copy_from_radio_model(settings_);

    loaded_ = load_settings(app_name_, bindings_);

    // Only copy to the radio if load was successful.
    if (loaded_)
        copy_to_radio_model(settings_);
}

SettingsManager::~SettingsManager() {
    copy_from_radio_model(settings_);

    save_settings(app_name_, bindings_);
}

}  // namespace app_settings
