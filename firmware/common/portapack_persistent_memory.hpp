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

#include "rf_path.hpp"
#include "touch.hpp"
#include "modems.hpp"
#include "serializer.hpp"

using namespace modems;
using namespace serializer;

namespace portapack {
    namespace persistent_memory {

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

        bool config_splash();
        bool hide_clock();
        bool clock_with_date();
        bool config_login();
        bool config_speaker();
        uint32_t config_backlight_timer();

        void set_config_splash(bool v);
        void set_clock_hidden(bool v);
        void set_clock_with_date(bool v);
        void set_config_login(bool v);
        void set_config_speaker(bool v); 
        void set_config_backlight_timer(uint32_t i);

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

        /* Search app */
        bool search_app_autosave_freqs();
        bool search_app_autostart_search();
        bool search_app_continuous();
        bool search_app_clear_output();
        bool search_app_load_freqs();
        bool search_app_load_ranges();
        bool search_app_update_ranges_when_searching();
        bool search_app_load_hamradios();
        void set_search_app_autosave_freqs(const bool v);
        void set_search_app_autostart_search(const bool v);
        void set_search_app_continuous(const bool v);
        void set_search_app_clear_output(const bool v);
        void set_search_app_load_freqs(const bool v);
        void set_search_app_load_ranges(const bool v);
        void set_search_app_update_ranges_when_searching(const bool v);
        void set_search_app_load_hamradios(const bool v );
        void dump_mem();

    } /* namespace persistent_memory */
} /* namespace portapack */

#endif/*__PORTAPACK_PERSISTENT_MEMORY_H__*/
