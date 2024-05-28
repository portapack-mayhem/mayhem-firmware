/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 Mark Thompson
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

#ifndef __PORTAPACK_PERSISTENT_MEMORY_H__
#define __PORTAPACK_PERSISTENT_MEMORY_H__

#include <cstdint>

#include "optional.hpp"

#include "rf_path.hpp"
#include "touch.hpp"
#include "modems.hpp"
#include "serializer.hpp"
#include "volume.hpp"
#include "config_mode.hpp"
#include "ui.hpp"

// persistent memory from/to sdcard flag file
#define PMEM_FILEFLAG u"PMEM_FILEFLAG"

// persistent memory from/to sdcard flag file
#define PMEM_SETTING_FILE u"pmem_settings"

#define PMEM_SIZE_BYTES 256  // total amount of pmem space in bytes, including checksum
#define PMEM_SIZE_WORDS (PMEM_SIZE_BYTES / 4)

using namespace modems;
using namespace serializer;
using namespace ui;

namespace portapack {

namespace persistent_memory {

enum backlight_timeout_t {
    Timeout5Sec = 0,
    Timeout15Sec = 1,
    Timeout30Sec = 2,
    Timeout60Sec = 3,
    Timeout180Sec = 4,
    Timeout300Sec = 5,
    Timeout600Sec = 6,
    Timeout3600Sec = 7,
};

struct backlight_config_t {
   public:
    backlight_config_t()
        : _timeout_enum(backlight_timeout_t::Timeout600Sec),
          _timeout_enabled(false) {
    }

    backlight_config_t(
        backlight_timeout_t timeout_enum,
        bool timeout_enabled)
        : _timeout_enum(timeout_enum),
          _timeout_enabled(timeout_enabled) {
    }

    bool timeout_enabled() const {
        return _timeout_enabled;
    }

    backlight_timeout_t timeout_enum() const {
        return _timeout_enum;
    }

    uint32_t timeout_seconds() const {
        switch (timeout_enum()) {
            case Timeout5Sec:
                return 5;
            case Timeout15Sec:
                return 15;
            case Timeout30Sec:
                return 30;
            case Timeout60Sec:
                return 60;
            case Timeout180Sec:
                return 180;
            case Timeout300Sec:
                return 300;
            default:
            case Timeout600Sec:
                return 600;
            case Timeout3600Sec:
                return 3600;
        }
    }

   private:
    backlight_timeout_t _timeout_enum;
    bool _timeout_enabled;
};

enum encoder_dial_sensitivity {
    DIAL_SENSITIVITY_NORMAL = 0,
    DIAL_SENSITIVITY_LOW = 1,
    DIAL_SENSITIVITY_HIGH = 2,
    NUM_DIAL_SENSITIVITY
};

typedef union {
    uint32_t v;
    struct {
        uint8_t start_which : 4;
        uint8_t start_weekday : 4;
        uint8_t start_month : 4;
        uint8_t end_which : 4;
        uint8_t end_weekday : 4;
        uint8_t end_month : 4;
        uint8_t UNUSED : 7;
        uint8_t dst_enabled : 1;
    } b;
} dst_config_t;
static_assert(sizeof(dst_config_t) == sizeof(uint32_t));

enum fake_brightness_level_options {
    BRIGHTNESS_50 = 1,
    BRIGHTNESS_25 = 2,
    BRIGHTNESS_12p5 = 3,  // 12p5 is 12.5
};

namespace cache {

/* Set values in cache to sensible defaults. */
void defaults();

/* Load cached settings from values in persistent RAM, replacing with defaults
 * if persistent RAM contents appear to be invalid. */
void init();

/* Calculate a check value for cached settings, and copy the check value and
 * settings into persistent RAM. Intended to be called periodically to update
 * persistent settings with current settings. */
void persist();

} /* namespace cache */

using ppb_t = int32_t;

rf::Frequency target_frequency();
void set_target_frequency(const rf::Frequency new_value);

volume_t headphone_volume();
void set_headphone_volume(volume_t new_value);

ppb_t correction_ppb();
void set_correction_ppb(const ppb_t new_value);

void set_touch_calibration(const touch::Calibration& new_value);
const touch::Calibration& touch_calibration();

serial_format_t serial_format();
void set_serial_format(const serial_format_t new_value);

int32_t tone_mix();
void set_tone_mix(const int32_t new_value);

int32_t afsk_mark_freq();
void set_afsk_mark(const int32_t new_value);

int32_t afsk_space_freq();
void set_afsk_space(const int32_t new_value);

int32_t modem_baudrate();
void set_modem_baudrate(const int32_t new_value);

uint8_t modem_repeat();
void set_modem_repeat(const uint32_t new_value);

uint32_t playing_dead();
void set_playing_dead(const uint32_t new_value);

uint32_t playdead_sequence();
void set_playdead_sequence(const uint32_t new_value);

bool stealth_mode();
void set_stealth_mode(const bool v);

uint8_t config_cpld();
void set_config_cpld(uint8_t i);

bool config_disable_external_tcxo();
bool config_sdcard_high_speed_io();
bool config_disable_config_mode();
bool beep_on_packets();

bool config_splash();
bool config_converter();
bool config_updown_converter();
int64_t config_converter_freq();
bool show_gui_return_icon();
bool show_bigger_qr_code();
bool hide_clock();
bool clock_with_date();
bool config_login();
bool config_audio_mute();
bool config_speaker_disable();
backlight_config_t config_backlight_timer();
bool disable_touchscreen();

void set_gui_return_icon(bool v);
void set_load_app_settings(bool v);
void set_save_app_settings(bool v);
void set_show_bigger_qr_code(bool v);
void set_config_disable_external_tcxo(bool v);
void set_config_sdcard_high_speed_io(bool v, bool save);
void set_config_disable_config_mode(bool v);
void set_beep_on_packets(bool v);

void set_config_splash(bool v);
bool config_converter();
bool config_updown_converter();
int64_t config_converter_freq();
void set_config_converter(bool v);
void set_config_updown_converter(bool v);
void set_config_converter_freq(int64_t v);
bool config_freq_tx_correction_updown();
void set_freq_tx_correction_updown(bool v);
bool config_freq_rx_correction_updown();
void set_freq_rx_correction_updown(bool v);
uint32_t config_freq_tx_correction();
uint32_t config_freq_rx_correction();
void set_config_freq_tx_correction(uint32_t v);
void set_config_freq_rx_correction(uint32_t v);
void set_clock_hidden(bool v);
void set_clock_with_date(bool v);
void set_config_login(bool v);
void set_config_audio_mute(bool v);
void set_config_speaker_disable(bool v);
void set_config_backlight_timer(const backlight_config_t& new_value);
void set_disable_touchscreen(bool v);

uint8_t encoder_dial_sensitivity();
void set_encoder_dial_sensitivity(uint8_t v);
uint8_t encoder_rate_multiplier();
void set_encoder_rate_multiplier(uint8_t v);

uint32_t config_mode_storage_direct();
void set_config_mode_storage_direct(uint32_t v);
bool config_disable_config_mode_direct();

uint32_t pocsag_last_address();
void set_pocsag_last_address(uint32_t address);

uint32_t pocsag_ignore_address();
void set_pocsag_ignore_address(uint32_t address);

bool clkout_enabled();
void set_clkout_enabled(bool v);
void set_clkout_freq(uint16_t freq);

bool dst_enabled();
void set_dst_enabled(bool v);
uint16_t clkout_freq();
dst_config_t config_dst();
void set_config_dst(dst_config_t v);

/* Fake brightness */
// switch (if do color change):
bool apply_fake_brightness();
void set_apply_fake_brightness(const bool v);
// level (color change level):
uint8_t fake_brightness_level();
void set_fake_brightness_level(uint8_t v);
void toggle_fake_brightness_level();

Color menu_color();
void set_menu_color(Color v);

/* Recon app */
bool recon_autosave_freqs();
bool recon_autostart_recon();
bool recon_continuous();
bool recon_clear_output();
bool recon_load_freqs();
bool recon_load_repeaters();
bool recon_load_ranges();
bool recon_update_ranges_when_recon();
bool recon_auto_record_locked();
bool recon_repeat_recorded();
bool recon_repeat_recorded_file_mode();
int8_t recon_repeat_nb();
int8_t recon_repeat_gain();
bool recon_repeat_amp();
bool recon_load_hamradios();
bool recon_match_mode();
uint8_t recon_repeat_delay();
void set_recon_autosave_freqs(const bool v);
void set_recon_autostart_recon(const bool v);
void set_recon_continuous(const bool v);
void set_recon_clear_output(const bool v);
void set_recon_load_freqs(const bool v);
void set_recon_load_ranges(const bool v);
void set_recon_update_ranges_when_recon(const bool v);
void set_recon_auto_record_locked(const bool v);
void set_recon_repeat_recorded(const bool v);
void set_recon_repeat_recorded_file_mode(const bool v);
void set_recon_repeat_nb(const int8_t v);
void set_recon_repeat_gain(const int8_t v);
void set_recon_repeat_amp(const bool v);
void set_recon_load_hamradios(const bool v);
void set_recon_load_repeaters(const bool v);
void set_recon_match_mode(const bool v);
void set_recon_repeat_delay(const uint8_t v);

/* UI Config 2 */
bool ui_hide_speaker();
bool ui_hide_mute();
bool ui_hide_converter();
bool ui_hide_stealth();
bool ui_hide_camera();
bool ui_hide_sleep();
bool ui_hide_bias_tee();
bool ui_hide_clock();
bool ui_hide_fake_brightness();
bool ui_hide_numeric_battery();
bool ui_hide_battery_icon();
bool ui_hide_sd_card();
uint8_t ui_theme_id();
void set_ui_hide_speaker(bool v);
void set_ui_hide_mute(bool v);
void set_ui_hide_converter(bool v);
void set_ui_hide_stealth(bool v);
void set_ui_hide_camera(bool v);
void set_ui_hide_sleep(bool v);
void set_ui_hide_bias_tee(bool v);
void set_ui_hide_clock(bool v);
void set_ui_hide_fake_brightness(bool v);
void set_ui_hide_numeric_battery(bool v);
void set_ui_hide_battery_icon(bool v);
void set_ui_hide_sd_card(bool v);
void set_ui_theme_id(uint8_t v);

// sd persisting settings
bool should_use_sdcard_for_pmem();
int save_persistent_settings_to_file();
int load_persistent_settings_from_file();

uint32_t pmem_data_word(uint32_t index);
uint32_t pmem_stored_checksum(void);
uint32_t pmem_calculated_checksum(void);

size_t data_size();

bool debug_dump();

} /* namespace persistent_memory */

} /* namespace portapack */

#endif /*__PORTAPACK_PERSISTENT_MEMORY_H__*/
