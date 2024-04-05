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

// Bring in the string_view literal.
using std::literals::operator""sv;

#define COMMON_APP_SETTINGS_COUNT 19

/* Represents a named setting bound to a variable instance. */
/* Using void* instead of std::variant, because variant is a pain to dispatch over. */
class BoundSetting {
    /* The type of bound setting. */
    enum class SettingType : uint8_t {
        I64,
        I32,
        U32,
        U8,
        String,
        Bool,
    };

   public:
    BoundSetting(std::string_view name, int64_t* target)
        : name_{name}, target_{target}, type_{SettingType::I64} {}

    BoundSetting(std::string_view name, int32_t* target)
        : name_{name}, target_{target}, type_{SettingType::I32} {}

    BoundSetting(std::string_view name, uint32_t* target)
        : name_{name}, target_{target}, type_{SettingType::U32} {}

    BoundSetting(std::string_view name, uint8_t* target)
        : name_{name}, target_{target}, type_{SettingType::U8} {}

    BoundSetting(std::string_view name, std::string* target)
        : name_{name}, target_{target}, type_{SettingType::String} {}

    BoundSetting(std::string_view name, bool* target)
        : name_{name}, target_{target}, type_{SettingType::Bool} {}

    std::string_view name() const { return name_; }
    void parse(std::string_view value);
    void write(File& file) const;

   private:
    template <typename T>
    constexpr auto& as() const {
        return *reinterpret_cast<T*>(target_);
    }

    std::string_view name_;
    void* target_;
    SettingType type_;
};

using SettingBindings = std::vector<BoundSetting>;

/* RAII wrapper for Settings that loads/saves to the SD card. */
class SettingsStore {
   public:
    SettingsStore(std::string_view store_name, SettingBindings bindings);
    ~SettingsStore();

    void reload();
    void save() const;

   private:
    std::string_view store_name_;
    SettingBindings bindings_;
};

bool load_settings(std::string_view store_name, SettingBindings& bindings);
bool save_settings(std::string_view store_name, const SettingBindings& bindings);

namespace app_settings {

enum class Mode : uint8_t {
    NO_RF = 0x00,
    RX = 0x01,
    TX = 0x02,
    RX_TX = 0x03,  // Both TX/RX
};

enum class Options {
    None = 0x0000,
};

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
    // NOTE: update COMMON_APP_SETTINGS_COUNT when adding to this
};

/* Copies common values to the receiver/transmitter models. */
void copy_to_radio_model(const AppSettings& settings);

/* Copies common values from the receiver/transmitter models. */
void copy_from_radio_model(AppSettings& settings);

/* RAII wrapper for automatically loading and saving settings for an app.
 * "Additional" settings are always loaded, but radio settings are conditionally
 * saved/loaded if the global app settings options are enabled.
 * NB: This should be added to a class before any LNA/VGA controls so that
 * the receiver/transmitter models are set before the control ctors run. */
class SettingsManager {
   public:
    SettingsManager(std::string_view app_name, Mode mode, Options options = Options::None);
    SettingsManager(std::string_view app_name, Mode mode, SettingBindings additional_settings);
    SettingsManager(std::string_view app_name, Mode mode, Options options, SettingBindings additional_settings);
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
    std::string_view app_name_;
    AppSettings settings_;
    SettingBindings bindings_;
    bool loaded_;
};

}  // namespace app_settings

ENABLE_FLAGS_OPERATORS(app_settings::Mode);
ENABLE_FLAGS_OPERATORS(app_settings::Options);

#endif /*__APP_SETTINGS_H__*/
