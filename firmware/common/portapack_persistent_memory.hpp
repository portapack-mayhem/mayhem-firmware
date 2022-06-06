/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

using namespace modems;
using namespace serializer;

namespace portapack {
namespace persistent_memory {

enum backlight_timeout_t {
    Timeout5Sec    = 0,
    Timeout15Sec   = 1,
    Timeout30Sec   = 2,
    Timeout60Sec   = 3,
    Timeout180Sec  = 4,
    Timeout300Sec  = 5,
    Timeout600Sec  = 6,
    Timeout3600Sec = 7,
};

struct backlight_config_t {
public:
    backlight_config_t() :
        _timeout_enum(backlight_timeout_t::Timeout600Sec),
        _timeout_enabled(false)
    {
    }

    backlight_config_t(
        backlight_timeout_t timeout_enum,
        bool timeout_enabled
    ) :
        _timeout_enum(timeout_enum),
        _timeout_enabled(timeout_enabled)
    {
    }

    bool timeout_enabled() const {
        return _timeout_enabled;
    }

    backlight_timeout_t timeout_enum() const {
        return _timeout_enum;
    }

    uint32_t timeout_seconds() const {
        switch(timeout_enum()) {
            case Timeout5Sec:    return    5;
            case Timeout15Sec:   return   15;
            case Timeout30Sec:   return   30;
            case Timeout60Sec:   return   60;
            case Timeout180Sec:  return  180;
            case Timeout300Sec:  return  300;
            default:
            case Timeout600Sec:  return  600;
            case Timeout3600Sec: return 3600;
        }
    }

private:
    backlight_timeout_t _timeout_enum;
    bool _timeout_enabled;
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

rf::Frequency tuned_frequency();
void set_tuned_frequency(const rf::Frequency new_value);

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

bool config_splash();
bool show_gui_return_icon();
bool load_app_settings();
bool save_app_settings();
bool show_bigger_qr_code();
bool hide_clock();
bool clock_with_date();
bool config_login();
bool config_speaker();
backlight_config_t config_backlight_timer();
bool disable_touchscreen();

void set_gui_return_icon(bool v);
void set_load_app_settings(bool v);
void set_save_app_settings(bool v);
void set_show_bigger_qr_code(bool v);
void set_config_splash(bool v);
void set_clock_hidden(bool v);
void set_clock_with_date(bool v);
void set_config_login(bool v);
void set_config_speaker(bool v); 
void set_config_backlight_timer(const backlight_config_t& new_value);
void set_disable_touchscreen(bool v);

//uint8_t ui_config_textentry();
//void set_config_textentry(uint8_t new_value);

uint32_t pocsag_last_address();
void set_pocsag_last_address(uint32_t address);

uint32_t pocsag_ignore_address();
void set_pocsag_ignore_address(uint32_t address);

bool clkout_enabled();
void set_clkout_enabled(bool v);
uint32_t clkout_freq();
void set_clkout_freq(uint32_t freq);

} /* namespace persistent_memory */
} /* namespace portapack */

#endif/*__PORTAPACK_PERSISTENT_MEMORY_H__*/
