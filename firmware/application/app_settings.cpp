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

#include "app_settings.hpp"

#include "file.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cstring>
#include <string_view>

namespace fs = std::filesystem;
using namespace portapack;
using namespace std::literals;

namespace app_settings {

template <typename T>
static void read_setting(
    std::string_view file_content,
    std::string_view setting_name,
    T& value) {
    auto pos = file_content.find(setting_name);

    if (pos != file_content.npos) {
        pos += setting_name.length();
        value = strtoll(&file_content[pos], nullptr, 10);
    }
}

template <typename T>
static void write_setting(File& file, std::string_view setting_name, const T& value) {
    // NB: Not using file.write_line speed this up. This happens on every
    // app exit when enabled so should be fast to keep the UX responsive.
    StringFormatBuffer buffer;
    size_t length = 0;
    auto value_str = to_string_dec_uint(value, buffer, length);

    file.write(setting_name.data(), setting_name.length());
    file.write(value_str, length);
    file.write("\r\n", 2);
}

static fs::path get_settings_path(const std::string& app_name) {
    return fs::path{u"/SETTINGS"} / app_name + u".ini";
}

namespace setting {
constexpr std::string_view baseband_bandwidth = "baseband_bandwidth="sv;
constexpr std::string_view sampling_rate = "sampling_rate="sv;
constexpr std::string_view lna = "lna="sv;
constexpr std::string_view vga = "vga="sv;
constexpr std::string_view rx_amp = "rx_amp="sv;
constexpr std::string_view tx_amp = "tx_amp="sv;
constexpr std::string_view tx_gain = "tx_gain="sv;
constexpr std::string_view channel_bandwidth = "channel_bandwidth="sv;
constexpr std::string_view rx_frequency = "rx_frequency="sv;
constexpr std::string_view tx_frequency = "tx_frequency="sv;
constexpr std::string_view step = "step="sv;
constexpr std::string_view modulation = "modulation="sv;
constexpr std::string_view am_config_index = "am_config_index="sv;
constexpr std::string_view nbfm_config_index = "nbfm_config_index="sv;
constexpr std::string_view wfm_config_index = "wfm_config_index="sv;
constexpr std::string_view squelch = "squelch="sv;
constexpr std::string_view volume = "volume="sv;
}  // namespace setting

// TODO: Only load/save values that are declared used.
// This will prevent switching apps from changing setting unnecessarily.
// TODO: Track which values are actually read.
// TODO: Maybe just use a dictionary which would allow for custom settings.
// TODO: Create a control value binding which will allow controls to
//       be declaratively bound to a setting and persistence will be magic.
// TODO: radio settings should be pushed and popped to prevent cross-app
//       radio bugs caused by sharing a global model.

ResultCode load_settings(const std::string& app_name, AppSettings& settings) {
    if (!portapack::persistent_memory::load_app_settings())
        return ResultCode::SettingsDisabled;

    auto file_path = get_settings_path(app_name);
    auto data = File::read_file(file_path);

    if (!data)
        return ResultCode::LoadFailed;

    if (flags_enabled(settings.mode, Mode::TX)) {
        read_setting(*data, setting::tx_frequency, settings.tx_frequency);
        read_setting(*data, setting::tx_amp, settings.tx_amp);
        read_setting(*data, setting::tx_gain, settings.tx_gain);
        read_setting(*data, setting::channel_bandwidth, settings.channel_bandwidth);
    }

    if (flags_enabled(settings.mode, Mode::RX)) {
        read_setting(*data, setting::rx_frequency, settings.rx_frequency);
        read_setting(*data, setting::rx_amp, settings.rx_amp);
        read_setting(*data, setting::modulation, settings.modulation);
        read_setting(*data, setting::am_config_index, settings.am_config_index);
        read_setting(*data, setting::nbfm_config_index, settings.nbfm_config_index);
        read_setting(*data, setting::wfm_config_index, settings.wfm_config_index);
        read_setting(*data, setting::squelch, settings.squelch);
    }

    read_setting(*data, setting::baseband_bandwidth, settings.baseband_bandwidth);
    read_setting(*data, setting::sampling_rate, settings.sampling_rate);
    read_setting(*data, setting::lna, settings.lna);
    read_setting(*data, setting::vga, settings.vga);
    read_setting(*data, setting::step, settings.step);
    read_setting(*data, setting::volume, settings.volume);

    return ResultCode::Ok;
}

ResultCode save_settings(const std::string& app_name, AppSettings& settings) {
    if (!portapack::persistent_memory::save_app_settings())
        return ResultCode::SettingsDisabled;

    File settings_file;
    auto file_path = get_settings_path(app_name);
    ensure_directory(file_path.parent_path());

    auto error = settings_file.create(file_path);
    if (error)
        return ResultCode::SaveFailed;

    if (flags_enabled(settings.mode, Mode::TX)) {
        write_setting(settings_file, setting::tx_frequency, settings.tx_frequency);
        write_setting(settings_file, setting::tx_amp, settings.tx_amp);
        write_setting(settings_file, setting::tx_gain, settings.tx_gain);
        write_setting(settings_file, setting::channel_bandwidth, settings.channel_bandwidth);
    }

    if (flags_enabled(settings.mode, Mode::RX)) {
        write_setting(settings_file, setting::rx_frequency, settings.rx_frequency);
        write_setting(settings_file, setting::rx_amp, settings.rx_amp);
        write_setting(settings_file, setting::modulation, settings.modulation);
        write_setting(settings_file, setting::am_config_index, settings.am_config_index);
        write_setting(settings_file, setting::nbfm_config_index, settings.nbfm_config_index);
        write_setting(settings_file, setting::wfm_config_index, settings.wfm_config_index);
        write_setting(settings_file, setting::squelch, settings.squelch);
    }

    write_setting(settings_file, setting::baseband_bandwidth, settings.baseband_bandwidth);
    write_setting(settings_file, setting::sampling_rate, settings.sampling_rate);
    write_setting(settings_file, setting::lna, settings.lna);
    write_setting(settings_file, setting::vga, settings.vga);
    write_setting(settings_file, setting::step, settings.step);
    write_setting(settings_file, setting::volume, settings.volume);

    return ResultCode::Ok;
}

void copy_to_radio_model(const AppSettings& settings) {
    // NB: Don't actually adjust the radio here or it will hang.

    if (flags_enabled(settings.mode, Mode::TX)) {
        if (!flags_enabled(settings.options, Options::UseGlobalTargetFrequency))
            transmitter_model.set_target_frequency(settings.tx_frequency);

        transmitter_model.configure_from_app_settings(settings);
    }

    if (flags_enabled(settings.mode, Mode::RX)) {
        if (!flags_enabled(settings.options, Options::UseGlobalTargetFrequency))
            transmitter_model.set_target_frequency(settings.rx_frequency);

        receiver_model.configure_from_app_settings(settings);
        receiver_model.set_configuration_without_init(
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
        settings.tx_amp = transmitter_model.rf_amp();
        settings.tx_gain = transmitter_model.tx_gain();
        settings.channel_bandwidth = transmitter_model.channel_bandwidth();

        // TODO: Do these make sense for TX?
        settings.sampling_rate = transmitter_model.sampling_rate();
        settings.lna = transmitter_model.lna();
        settings.vga = transmitter_model.vga();
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
SettingsManager::SettingsManager(
    std::string app_name,
    Mode mode,
    Options options)
    : app_name_{std::move(app_name)},
      settings_{},
      loaded_{false} {
    settings_.mode = mode;
    settings_.options = options;
    auto result = load_settings(app_name_, settings_);

    if (result == ResultCode::Ok) {
        loaded_ = true;
        copy_to_radio_model(settings_);
    }
}

SettingsManager::~SettingsManager() {
    if (portapack::persistent_memory::save_app_settings()) {
        copy_from_radio_model(settings_);
        save_settings(app_name_, settings_);
    }
}

}  // namespace app_settings
