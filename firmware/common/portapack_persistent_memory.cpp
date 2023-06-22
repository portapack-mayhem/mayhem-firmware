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

#include "portapack_persistent_memory.hpp"

#include "audio.hpp"
#include "portapack.hpp"
#include "hal.h"

#include "utility.hpp"

#include "memory_map.hpp"

#include "crc.hpp"

#include <algorithm>
#include <utility>

#include <string>
#include <fstream>
#include "file.hpp"

#include "irq_controls.hpp"

#include "string_format.hpp"
#include "ui_styles.hpp"
#include "ui_painter.hpp"

#include <ch.h>

using namespace std;

namespace portapack {
namespace persistent_memory {

constexpr rf::Frequency target_frequency_reset_value{100000000};

using ppb_range_t = range_t<ppb_t>;
constexpr ppb_range_t ppb_range{-99000, 99000};
constexpr ppb_t ppb_reset_value{0};

using tone_mix_range_t = range_t<int32_t>;
constexpr tone_mix_range_t tone_mix_range{10, 99};
constexpr int32_t tone_mix_reset_value{20};

using afsk_freq_range_t = range_t<int32_t>;
constexpr afsk_freq_range_t afsk_freq_range{1, 4000};
constexpr int32_t afsk_mark_reset_value{1200};
constexpr int32_t afsk_space_reset_value{2200};

using modem_baudrate_range_t = range_t<int32_t>;
constexpr modem_baudrate_range_t modem_baudrate_range{50, 9600};
constexpr int32_t modem_baudrate_reset_value{1200};

/*using modem_bw_range_t = range_t<int32_t>;
                  constexpr modem_bw_range_t modem_bw_range { 1000, 50000 };
                  constexpr int32_t modem_bw_reset_value { 15000 };*/

using modem_repeat_range_t = range_t<int32_t>;
constexpr modem_repeat_range_t modem_repeat_range{1, 99};
constexpr int32_t modem_repeat_reset_value{5};

using clkout_freq_range_t = range_t<uint32_t>;
constexpr clkout_freq_range_t clkout_freq_range{10, 60000};
constexpr uint32_t clkout_freq_reset_value{10000};

enum data_structure_version_enum : uint32_t {
    VERSION_CURRENT = 0x10000003,
};

static const uint32_t TOUCH_CALIBRATION_MAGIC = 0x074af82f;

#define _bit_write(__value, __bit, _v)     \
    if (_bit_read(__value, __bit) != _v) { \
        __value ^= 1 << __bit;             \
    }

#define _bit_read(__value, __bit) (((__value >> __bit) & 1) != 0)

enum bits_t {
    BacklightTimeoutLSB = 0,
    BacklightTimeoutEnable = 3,
    ClkoutFreqLSB = 4,
    ShowGUIReturnIcon = 20,
    LoadAppSettings = 21,
    SaveAppSettings = 22,
    ShowBiggerQRCode = 23,
    DisableTouchscreen = 24,
    HideClock = 25,
    ClockWithDate = 26,
    ClkOutEnabled = 27,
    ConfigSpeakerHidden = 28,  // unused since Speaker icon modifications
    StealthMode = 29,
    ConfigLogin = 30,
    ConfigSplash = 31,
};
>>>>>>> 0c7571382021992a5571e6370c9e8b87fcec82fe

struct ui_config_t {
   private:
    enum bits_mask_t : uint32_t {
        BacklightTimeoutMask = ((1 << 3) - 1) << bits_t::BacklightTimeoutLSB,
        ClkoutFreqMask = ((1 << 16) - 1) << bits_t::ClkoutFreqLSB,
    };

    uint32_t values;

    constexpr bool bit_read(const bits_t n) const {
        return ((values >> n) & 1) != 0;
    }

    constexpr void bit_write(const bits_t n, const bool v) {
        if (bit_read(n) != v) {
            values ^= 1 << n;
        }
    }

   public:
    backlight_config_t config_backlight_timer() {
        const auto timeout_enum = (backlight_timeout_t)((values & bits_mask_t::BacklightTimeoutMask) >> bits_t::BacklightTimeoutLSB);
        const bool timeout_enabled = bit_read(bits_t::BacklightTimeoutEnable);
        return backlight_config_t(timeout_enum, timeout_enabled);
    }

    void set_config_backlight_timer(const backlight_config_t& new_value) {
        values = (values & ~bits_mask_t::BacklightTimeoutMask) | ((new_value.timeout_enum() << bits_t::BacklightTimeoutLSB) & bits_mask_t::BacklightTimeoutMask);
        bit_write(bits_t::BacklightTimeoutEnable, new_value.timeout_enabled());
    }

    constexpr uint32_t clkout_freq() {
        uint32_t freq = (values & bits_mask_t::ClkoutFreqMask) >> bits_t::ClkoutFreqLSB;
        if (freq < clkout_freq_range.minimum || freq > clkout_freq_range.maximum) {
            values = (values & ~bits_mask_t::ClkoutFreqMask) | (clkout_freq_reset_value << bits_t::ClkoutFreqLSB);
            return clkout_freq_reset_value;
        } else {
            return freq;
        }
    }

    constexpr void set_clkout_freq(uint32_t freq) {
        values = (values & ~bits_mask_t::ClkoutFreqMask) | (clkout_freq_range.clip(freq) << bits_t::ClkoutFreqLSB);
    }

    // ui_config is an uint32_t var storing information bitwise
    // bits 0-2 store the backlight timer
    // bits 4-19 (16 bits) store the clkout frequency
    // bits 21-31 store the different single bit configs depicted below
    // bit 20 store the display state of the gui return icon, hidden (0) or shown (1)

    constexpr bool show_gui_return_icon() const {  // add return icon in touchscreen menue
        return bit_read(bits_t::ShowGUIReturnIcon);
    }

    constexpr void set_gui_return_icon(bool v) {
        bit_write(bits_t::ShowGUIReturnIcon, v);
    }

    constexpr bool load_app_settings() const {  // load (last saved) app settings on startup of app
        return bit_read(bits_t::LoadAppSettings);
    }

    constexpr void set_load_app_settings(bool v) {
        bit_write(bits_t::LoadAppSettings, v);
    }

    constexpr bool save_app_settings() const {  // save app settings when closing app
        return bit_read(bits_t::SaveAppSettings);
    }

    constexpr void set_save_app_settings(bool v) {
        bit_write(bits_t::SaveAppSettings, v);
    }

    constexpr bool show_bigger_qr_code() const {  // show bigger QR code
        return bit_read(bits_t::ShowBiggerQRCode);
    }

    constexpr void set_show_bigger_qr_code(bool v) {
        bit_write(bits_t::ShowBiggerQRCode, v);
    }

    constexpr bool disable_touchscreen() const {  // Option to disable touch screen
        return bit_read(bits_t::DisableTouchscreen);
    }

    constexpr void set_disable_touchscreen(bool v) {
        bit_write(bits_t::DisableTouchscreen, v);
    }

    constexpr bool hide_clock() const {  // clock hidden from main menu
        return bit_read(bits_t::HideClock);
    }

    constexpr void set_clock_hidden(bool v) {
        bit_write(bits_t::HideClock, v);
    }

    constexpr bool clock_with_date() const {  // show clock with date, if not hidden
        return bit_read(bits_t::ClockWithDate);
    }

    constexpr void set_clock_with_date(bool v) {
        bit_write(bits_t::ClockWithDate, v);
    }

    constexpr bool clkout_enabled() const {
        return bit_read(bits_t::ClkOutEnabled);
    }

    constexpr void set_clkout_enabled(bool v) {
        bit_write(bits_t::ClkOutEnabled, v);
    }

    constexpr bool stealth_mode() const {
        return bit_read(bits_t::StealthMode);
    }

    constexpr void set_stealth_mode(bool v) {
        bit_write(bits_t::StealthMode, v);
    }

    constexpr bool config_login() const {
        return bit_read(bits_t::ConfigLogin);
    }

    constexpr void set_config_login(bool v) {
        bit_write(bits_t::ConfigLogin, v);
    }

    constexpr bool config_splash() const {
        return bit_read(bits_t::ConfigSplash);
    }

    constexpr void set_config_splash(bool v) {
        bit_write(bits_t::ConfigSplash, v);
    }

    constexpr ui_config_t()
        : values(
              (1 << ConfigSplash) | (clkout_freq_reset_value << ClkoutFreqLSB) | (7 << BacklightTimeoutLSB)) {
    }
};

struct misc_config_t {
   private:
    enum bits_t {
        ConfigAudioMute = 0,
        ConfigSpeakerDisable = 1,
    };

    // misc_config_t bits:
    // ConfigAudioMute = set to mute all audio output (speakers & headphones)
    // ConfigSpeakerDisable = set to disable only the speaker and leave headphones enabled (only supported on AK4951 codec)

    uint32_t values;

    constexpr bool bit_read(const bits_t n) const {
        return ((values >> n) & 1) != 0;
    }

    constexpr void bit_write(const bits_t n, const bool v) {
        if (bit_read(n) != v) {
            values ^= 1 << n;
        }
    }

   public:
    constexpr bool config_audio_mute() const {
        return bit_read(bits_t::ConfigAudioMute);
    }

    constexpr void set_config_audio_mute(bool v) {
        bit_write(bits_t::ConfigAudioMute, v);
    }

    constexpr bool config_speaker_disable() const {
        return bit_read(bits_t::ConfigSpeakerDisable);
    }

    constexpr void set_config_speaker_disable(bool v) {
        bit_write(bits_t::ConfigSpeakerDisable, v);
    }

    constexpr misc_config_t()
        : values(0) {
    }
};

/* IMPORTANT: Report your changes here in the dump_persistent_memory function a few lines later !! */

/* struct must pack the same way on M4 and M0 cores. */
struct data_t {
    data_structure_version_enum structure_version;
    int64_t target_frequency;
    int32_t correction_ppb;
    uint32_t touch_calibration_magic;
    touch::Calibration touch_calibration;

    // Modem
    uint32_t modem_def_index;
    serial_format_t serial_format;
    int32_t modem_bw;
    int32_t afsk_mark_freq;
    int32_t afsk_space_freq;
    int32_t modem_baudrate;
    int32_t modem_repeat;

    // Play dead unlock
    uint32_t playdead_magic;
    uint32_t playing_dead;
    uint32_t playdead_sequence;

    // UI
    ui_config_t ui_config;

    uint32_t pocsag_last_address;
    uint32_t pocsag_ignore_address;

    int32_t tone_mix;

    // Hardware
    uint32_t hardware_config;

    // Recon App
    uint8_t recon_config;

    // converter: show or hide icon. Hiding cause auto disable to avoid mistakes
    bool hide_converter;
    // enable or disable converter
    bool converter;
    // set up converter (false) or down converter (true) converter
    bool updown_converter;
    // up/down converter offset
    int64_t converter_frequency_offset;

    // frequency correction
    uint32_t frequency_rx_correction;
    bool updown_frequency_rx_correction;
    uint32_t frequency_tx_correction;
    bool updown_frequency_tx_correction;

    // Rotary encoder dial sensitivity (encoder.cpp/hpp)
    uint8_t encoder_dial_sensitivity;

    // Headphone volume in centibels.
    int32_t headphone_volume_cb;

    // Misc flags
    misc_config_t misc_config;

    constexpr data_t()
        : structure_version(data_structure_version_enum::VERSION_CURRENT),
          target_frequency(target_frequency_reset_value),
          correction_ppb(ppb_reset_value),
          touch_calibration_magic(TOUCH_CALIBRATION_MAGIC),
          touch_calibration(touch::Calibration()),

          modem_def_index(0),  // TODO: Unused?
          serial_format(),
          modem_bw(15000),  // TODO: Unused?
          afsk_mark_freq(afsk_mark_reset_value),
          afsk_space_freq(afsk_space_reset_value),
          modem_baudrate(modem_baudrate_reset_value),
          modem_repeat(modem_repeat_reset_value),

          playdead_magic(),     // TODO: Unused?
          playing_dead(),       // TODO: Unused?
          playdead_sequence(),  // TODO: Unused?

          ui_config(),

          pocsag_last_address(0),    // TODO: A better default?
          pocsag_ignore_address(0),  // TODO: A better default?

          tone_mix(tone_mix_reset_value),

          hardware_config(0),
          recon_config(0),
          hide_converter(0),
          converter(0),
          updown_converter(0),
          converter_frequency_offset(0),
          frequency_rx_correction(0),
          updown_frequency_rx_correction(0),
          frequency_tx_correction(0),
          updown_frequency_tx_correction(0),
          encoder_dial_sensitivity(0),
          headphone_volume_cb(-600),
          misc_config() {
    }
};

struct backup_ram_t {
   private:
    volatile uint32_t regfile[63];
    volatile uint32_t check_value;

    static void copy(const backup_ram_t& src, backup_ram_t& dst) {
        for (size_t i = 0; i < 63; i++) {
            dst.regfile[i] = src.regfile[i];
        }
        dst.check_value = src.check_value;
    }

    static void copy_from_data_t(const data_t& src, backup_ram_t& dst) {
        const uint32_t* const src_words = (uint32_t*)&src;
        const size_t word_count = (sizeof(data_t) + 3) / 4;
        for (size_t i = 0; i < 63; i++) {
            if (i < word_count) {
                dst.regfile[i] = src_words[i];
            } else {
                dst.regfile[i] = 0;
            }
        }
    }

    uint32_t compute_check_value() {
        CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};
        for (size_t i = 0; i < 63; i++) {
            const auto word = regfile[i];
            crc.process_byte((word >> 0) & 0xff);
            crc.process_byte((word >> 8) & 0xff);
            crc.process_byte((word >> 16) & 0xff);
            crc.process_byte((word >> 24) & 0xff);
        }
        return crc.checksum();
    }

   public:
    /* default constructor */
    backup_ram_t()
        : check_value(0) {
        const data_t defaults = data_t();
        copy_from_data_t(defaults, *this);
    }

    /* copy-assignment operator */
    backup_ram_t& operator=(const backup_ram_t& src) {
        copy(src, *this);
        return *this;
    }

    /* Calculate a check value from `this`, and check against
     * the stored value.
     */
    bool is_valid() {
        return compute_check_value() == check_value;
    }

    /* Assuming `this` contains valid data, update the checksum
     * and copy to the destination.
     */
    void persist_to(backup_ram_t& dst) {
        check_value = compute_check_value();
        copy(*this, dst);
    }
};

static_assert(sizeof(backup_ram_t) == memory::map::backup_ram.size());
static_assert(sizeof(data_t) <= sizeof(backup_ram_t) - sizeof(uint32_t));

static backup_ram_t* const backup_ram = reinterpret_cast<backup_ram_t*>(memory::map::backup_ram.base());

static backup_ram_t cached_backup_ram;
static data_t* data = reinterpret_cast<data_t*>(&cached_backup_ram);

namespace cache {

void defaults() {
    cached_backup_ram = backup_ram_t();
    *data = data_t();  // This is a workaround for apparently alignment issue
                       // that is causing backup_ram_t's block copy to be
                       // misaligned. This force sets values through the struct.

    // defaults values for recon app
    set_recon_autosave_freqs(false);
    set_recon_autostart_recon(true);
    set_recon_continuous(true);
    set_recon_clear_output(false);
    set_recon_load_freqs(true);
    set_recon_load_ranges(true);
    set_recon_update_ranges_when_recon(true);
    set_recon_load_hamradios(true);
    set_recon_match_mode(0);
}

void init() {
    const auto switches_state = get_switches_state();
    if (!(switches_state[(size_t)ui::KeyEvent::Left] && switches_state[(size_t)ui::KeyEvent::Right]) && backup_ram->is_valid()) {
        // Copy valid persistent data into cache.
        cached_backup_ram = *backup_ram;

        // Check that structure data we copied into cache is the expected
        // version. If not, initialize cache to defaults.
        if (data->structure_version != data_structure_version_enum::VERSION_CURRENT) {
            // TODO: Can provide version-to-version upgrade functions here,
            // if we want to be fancy.
            defaults();
        }
    } else {
        // Copy defaults into cache.
        defaults();
    }
}

void persist() {
    cached_backup_ram.persist_to(*backup_ram);
}

} /* namespace cache */

rf::Frequency target_frequency() {
    rf::tuning_range.reset_if_outside(data->target_frequency, target_frequency_reset_value);
    return data->target_frequency;
}

void set_target_frequency(const rf::Frequency new_value) {
    data->target_frequency = rf::tuning_range.clip(new_value);
}

volume_t headphone_volume() {
    auto volume = volume_t::centibel(data->headphone_volume_cb);
    volume = audio::headphone::volume_range().limit(volume);
    return volume;
}

void set_headphone_volume(volume_t new_value) {
    new_value = audio::headphone::volume_range().limit(new_value);
    data->headphone_volume_cb = new_value.centibel();
}

ppb_t correction_ppb() {
    ppb_range.reset_if_outside(data->correction_ppb, ppb_reset_value);
    return data->correction_ppb;
}

void set_correction_ppb(const ppb_t new_value) {
    const auto clipped_value = ppb_range.clip(new_value);
    data->correction_ppb = clipped_value;
    portapack::clock_manager.set_reference_ppb(clipped_value);
}

void set_touch_calibration(const touch::Calibration& new_value) {
    data->touch_calibration = new_value;
    data->touch_calibration_magic = TOUCH_CALIBRATION_MAGIC;
}

const touch::Calibration& touch_calibration() {
    if (data->touch_calibration_magic != TOUCH_CALIBRATION_MAGIC) {
        set_touch_calibration(touch::Calibration());
    }
    return data->touch_calibration;
}

int32_t tone_mix() {
    tone_mix_range.reset_if_outside(data->tone_mix, tone_mix_reset_value);
    return data->tone_mix;
}

void set_tone_mix(const int32_t new_value) {
    data->tone_mix = tone_mix_range.clip(new_value);
}

int32_t afsk_mark_freq() {
    afsk_freq_range.reset_if_outside(data->afsk_mark_freq, afsk_mark_reset_value);
    return data->afsk_mark_freq;
}

void set_afsk_mark(const int32_t new_value) {
    data->afsk_mark_freq = afsk_freq_range.clip(new_value);
}

int32_t afsk_space_freq() {
    afsk_freq_range.reset_if_outside(data->afsk_space_freq, afsk_space_reset_value);
    return data->afsk_space_freq;
}

void set_afsk_space(const int32_t new_value) {
    data->afsk_space_freq = afsk_freq_range.clip(new_value);
}

int32_t modem_baudrate() {
    modem_baudrate_range.reset_if_outside(data->modem_baudrate, modem_baudrate_reset_value);
    return data->modem_baudrate;
}

void set_modem_baudrate(const int32_t new_value) {
    data->modem_baudrate = modem_baudrate_range.clip(new_value);
}

/*int32_t modem_bw() {
                  modem_bw_range.reset_if_outside(data->modem_bw, modem_bw_reset_value);
                  return data->modem_bw;
                  }

                  void set_modem_bw(const int32_t new_value) {
                  data->modem_bw = modem_bw_range.clip(new_value);
                  }*/

uint8_t modem_repeat() {
    modem_repeat_range.reset_if_outside(data->modem_repeat, modem_repeat_reset_value);
    return data->modem_repeat;
}

void set_modem_repeat(const uint32_t new_value) {
    data->modem_repeat = modem_repeat_range.clip(new_value);
}

serial_format_t serial_format() {
    return data->serial_format;
}

void set_serial_format(const serial_format_t new_value) {
    data->serial_format = new_value;
}

bool show_gui_return_icon() {  // add return icon in touchscreen menue
    return data->ui_config.show_gui_return_icon();
}

bool load_app_settings() {  // load (last saved) app settings on startup of app
    return data->ui_config.load_app_settings();
}

bool save_app_settings() {  // save app settings when closing app
    return data->ui_config.save_app_settings();
}

bool show_bigger_qr_code() {  // show bigger QR code
    return data->ui_config.show_bigger_qr_code();
}

bool disable_touchscreen() {  // Option to disable touch screen
    return data->ui_config.disable_touchscreen();
}

bool hide_clock() {  // clock hidden from main menu
    return data->ui_config.hide_clock();
}

bool clock_with_date() {  // show clock with date, if not hidden
    return data->ui_config.clock_with_date();
}

bool clkout_enabled() {
    return data->ui_config.clkout_enabled();
}

bool config_audio_mute() {
    return data->misc_config.config_audio_mute();
}

bool config_speaker_disable() {
    return data->misc_config.config_speaker_disable();
}

bool stealth_mode() {
    return data->ui_config.stealth_mode();
}

bool config_login() {
    return data->ui_config.config_login();
}

bool config_splash() {
    return data->ui_config.config_splash();
}

uint8_t config_cpld() {
    return data->hardware_config;
}

backlight_config_t config_backlight_timer() {
    return data->ui_config.config_backlight_timer();
}

void set_gui_return_icon(bool v) {
    data->ui_config.set_gui_return_icon(v);
}

void set_load_app_settings(bool v) {
    data->ui_config.set_load_app_settings(v);
}

void set_save_app_settings(bool v) {
    data->ui_config.set_save_app_settings(v);
}

void set_show_bigger_qr_code(bool v) {
    data->ui_config.set_show_bigger_qr_code(v);
}

void set_disable_touchscreen(bool v) {
    data->ui_config.set_disable_touchscreen(v);
}

void set_clock_hidden(bool v) {
    data->ui_config.set_clock_hidden(v);
}

void set_clock_with_date(bool v) {
    data->ui_config.set_clock_with_date(v);
}

void set_clkout_enabled(bool v) {
    data->ui_config.set_clkout_enabled(v);
}

void set_config_audio_mute(bool v) {
    data->misc_config.set_config_audio_mute(v);
}

void set_config_speaker_disable(bool v) {
    data->misc_config.set_config_speaker_disable(v);
}

void set_stealth_mode(bool v) {
    data->ui_config.set_stealth_mode(v);
}

void set_config_login(bool v) {
    data->ui_config.set_config_login(v);
}

void set_config_splash(bool v) {
    data->ui_config.set_config_splash(v);
}

void set_config_cpld(uint8_t i) {
    data->hardware_config = i;
}

void set_config_backlight_timer(const backlight_config_t& new_value) {
    data->ui_config.set_config_backlight_timer(new_value);
}

/*void set_config_textentry(uint8_t new_value) {
                  data->ui_config = (data->ui_config & ~0b100) | ((new_value & 1) << 2);
                  }

                  uint8_t ui_config_textentry() {
                  return ((data->ui_config >> 2) & 1);
                  }*/

/*void set_ui_config(const uint32_t new_value) {
                  data->ui_config = new_value;
                  }*/

uint32_t pocsag_last_address() {
    return data->pocsag_last_address;
}

void set_pocsag_last_address(uint32_t address) {
    data->pocsag_last_address = address;
}

uint32_t pocsag_ignore_address() {
    return data->pocsag_ignore_address;
}

void set_pocsag_ignore_address(uint32_t address) {
    data->pocsag_ignore_address = address;
}

uint32_t clkout_freq() {
    return data->ui_config.clkout_freq();
}

void set_clkout_freq(uint32_t freq) {
    data->ui_config.set_clkout_freq(freq);
}

bool recon_autosave_freqs() {
    return _bit_read(data->recon_config, 0);
}
bool recon_autostart_recon() {
    return _bit_read(data->recon_config, 1);
}
bool recon_continuous() {
    return _bit_read(data->recon_config, 2);
}
bool recon_clear_output() {
    return _bit_read(data->recon_config, 3);
}
bool recon_load_freqs() {
    return _bit_read(data->recon_config, 4);
}
bool recon_load_ranges() {
    return _bit_read(data->recon_config, 5);
}
bool recon_update_ranges_when_recon() {
    return _bit_read(data->recon_config, 6);
}
bool recon_load_hamradios() {
    return _bit_read(data->recon_config, 7);
}
bool recon_match_mode() {
    return _bit_read(data->recon_config, 8);
}
bool recon_auto_record_locked() {
    return _bit_read(data->recon_config, 9);
}

void set_recon_autosave_freqs(const bool v) {
    _bit_write(data->recon_config, 0, v);
}
void set_recon_autostart_recon(const bool v) {
    _bit_write(data->recon_config, 1, v);
    data->recon_config = (data->recon_config & ~0x40000000UL) | (v << 30);
}
void set_recon_continuous(const bool v) {
    _bit_write(data->recon_config, 2, v);
}
void set_recon_clear_output(const bool v) {
    _bit_write(data->recon_config, 3, v);
}
void set_recon_load_freqs(const bool v) {
    _bit_write(data->recon_config, 4, v);
}
void set_recon_load_ranges(const bool v) {
    _bit_write(data->recon_config, 5, v);
}
void set_recon_update_ranges_when_recon(const bool v) {
    _bit_write(data->recon_config, 6, v);
}
void set_recon_load_hamradios(const bool v) {
    _bit_write(data->recon_config, 7, v);
}
void set_recon_match_mode(const bool v) {
    _bit_write(data->recon_config, 8, v);
}
void set_recon_auto_record_locked(const bool v) {
    _bit_write(data->recon_config, 9, v);
}
bool config_hide_converter() {
    return data->hide_converter;
}
bool config_converter() {
    return data->converter;
}
bool config_updown_converter() {
    return data->updown_converter;
}
int64_t config_converter_freq() {
    return data->converter_frequency_offset;
}

void set_config_hide_converter(bool v) {
    data->hide_converter = v;
    if (v) {
        data->converter = false;
    }
}
void set_config_converter(bool v) {
    data->converter = v;
}
void set_config_updown_converter(bool v) {
    data->updown_converter = v;
}
void set_config_converter_freq(int64_t v) {
    data->converter_frequency_offset = v;
}

// frequency correction settings
bool config_freq_tx_correction_updown() {
    return data->updown_frequency_tx_correction;
}
void set_freq_tx_correction_updown(bool v) {
    data->updown_frequency_tx_correction = v;
}
bool config_freq_rx_correction_updown() {
    return data->updown_frequency_rx_correction;
}
void set_freq_rx_correction_updown(bool v) {
    data->updown_frequency_rx_correction = v;
}
uint32_t config_freq_tx_correction() {
    return data->frequency_tx_correction;
}
uint32_t config_freq_rx_correction() {
    return data->frequency_rx_correction;
}
void set_config_freq_tx_correction(uint32_t v) {
    data->frequency_tx_correction = v;
}
void set_config_freq_rx_correction(uint32_t v) {
    data->frequency_rx_correction = v;
}

// rotary encoder dial settings
uint8_t config_encoder_dial_sensitivity() {
    return data->encoder_dial_sensitivity;
}
void set_encoder_dial_sensitivity(uint8_t v) {
    data->encoder_dial_sensitivity = v;
}

bool should_use_sdcard_for_pmem() {
    return std::filesystem::file_exists(PMEM_FILEFLAG);
}

// sd persisting settings
int save_persistent_settings_to_file() {
    std::string filename = PMEM_SETTING_FILE;
    delete_file(filename);
    File outfile;
    auto result = outfile.create(filename);
    if (result.is_valid()) {
        return false;
    }
    outfile.write(reinterpret_cast<char*>(&cached_backup_ram), sizeof(backup_ram_t));
    return true;
}

int load_persistent_settings_from_file() {
    std::string filename = PMEM_SETTING_FILE;
    File infile;
    auto result = infile.open(filename);
    if (!result.is_valid()) {
        infile.read(reinterpret_cast<char*>(&cached_backup_ram), sizeof(backup_ram_t));
        return true;
    }
    return false;
}

size_t data_size() {
    return sizeof(data_t);
}

bool debug_dump() {
    ui::Painter painter{};
    std::string debug_dir = "DEBUG";
    std::filesystem::path filename{};
    File pmem_dump_file{};
    // create new dump file name and DEBUG directory
    make_new_directory(debug_dir);
    filename = next_filename_matching_pattern(debug_dir + "/DEBUG_DUMP_????.TXT");
    if (filename.empty()) {
        return false;
    }
    // dump data fo filename
    auto error = pmem_dump_file.create(filename);
    if (error) {
        painter.draw_string({0, 320 - 16}, ui::Styles::red, "ERROR DUMPING " + filename.filename().string() + " !");
        return false;
    }

    // write persistent memory
    pmem_dump_file.write_line("[Persistent Memory]");
    // full variables
    pmem_dump_file.write_line("structure_version: " + to_string_dec_uint(data->structure_version));
    pmem_dump_file.write_line("target_frequency: " + to_string_dec_int(data->target_frequency));
    pmem_dump_file.write_line("correction_ppb: " + to_string_dec_int(data->correction_ppb));
    pmem_dump_file.write_line("modem_def_index: " + to_string_dec_uint(data->modem_def_index));
    pmem_dump_file.write_line("serial_format.data_bit: " + to_string_dec_uint(data->serial_format.data_bits));
    pmem_dump_file.write_line("serial_format.parity: " + to_string_dec_uint(data->serial_format.parity));
    pmem_dump_file.write_line("serial_format.stop_bits: " + to_string_dec_uint(data->serial_format.stop_bits));
    pmem_dump_file.write_line("serial_format.bit_order: " + to_string_dec_uint(data->serial_format.bit_order));
    pmem_dump_file.write_line("modem_bw: " + to_string_dec_int(data->modem_bw));
    pmem_dump_file.write_line("afsk_mark_freq: " + to_string_dec_int(data->afsk_mark_freq));
    pmem_dump_file.write_line("afsk_space_freq: " + to_string_dec_int(data->afsk_space_freq));
    pmem_dump_file.write_line("modem_baudrate: " + to_string_dec_int(data->modem_baudrate));
    pmem_dump_file.write_line("modem_repeat: " + to_string_dec_int(data->modem_repeat));
    pmem_dump_file.write_line("playdead_magic: " + to_string_dec_uint(data->playdead_magic));
    pmem_dump_file.write_line("playing_dead: " + to_string_dec_uint(data->playing_dead));
    pmem_dump_file.write_line("playdead_sequence: " + to_string_dec_uint(data->playdead_sequence));
    pmem_dump_file.write_line("pocsag_last_address: " + to_string_dec_uint(data->pocsag_last_address));
    pmem_dump_file.write_line("pocsag_ignore_address: " + to_string_dec_uint(data->pocsag_ignore_address));
    pmem_dump_file.write_line("hardware_config: " + to_string_dec_uint(data->hardware_config));
    pmem_dump_file.write_line("recon_config: " + to_string_dec_uint(data->recon_config));
    pmem_dump_file.write_line("hide_converter: " + to_string_dec_int(data->tone_mix));
    pmem_dump_file.write_line("converter: " + to_string_dec_int(data->tone_mix));
    pmem_dump_file.write_line("updown_converter: " + to_string_dec_int(data->tone_mix));
    pmem_dump_file.write_line("frequency_rx_correction: " + to_string_dec_uint(data->frequency_rx_correction));
    pmem_dump_file.write_line("updown_frequency_rx_correction: " + to_string_dec_int(data->updown_frequency_rx_correction));
    pmem_dump_file.write_line("frequency_tx_correction: " + to_string_dec_uint(data->frequency_tx_correction));
    pmem_dump_file.write_line("updown_frequency_tx_correction: " + to_string_dec_int(data->updown_frequency_tx_correction));
    pmem_dump_file.write_line("encoder_dial_sensitivity: " + to_string_dec_uint(data->encoder_dial_sensitivity));
    pmem_dump_file.write_line("headphone_volume_cb: " + to_string_dec_int(data->headphone_volume_cb));
    // ui_config bits
    const auto backlight_timer = portapack::persistent_memory::config_backlight_timer();
    pmem_dump_file.write_line("ui_config backlight_timer.timeout_enabled: " + to_string_dec_uint(backlight_timer.timeout_enabled()));
    pmem_dump_file.write_line("ui_config backlight_timer.timeout_seconds: " + to_string_dec_uint(backlight_timer.timeout_seconds()));
    pmem_dump_file.write_line("ui_config clkout_freq: " + to_string_dec_uint(clkout_freq()));
    pmem_dump_file.write_line("ui_config show_gui_return_icon: " + to_string_dec_uint(data->ui_config.show_gui_return_icon()));
    pmem_dump_file.write_line("ui_config load_app_settings: " + to_string_dec_uint(data->ui_config.load_app_settings()));
    pmem_dump_file.write_line("ui_config save_app_settings: " + to_string_dec_uint(data->ui_config.save_app_settings()));
    pmem_dump_file.write_line("ui_config show_bigger_qr_code: " + to_string_dec_uint(data->ui_config.show_bigger_qr_code()));
    pmem_dump_file.write_line("ui_config disable_touchscreen: " + to_string_dec_uint(data->ui_config.disable_touchscreen()));
    pmem_dump_file.write_line("ui_config hide_clock: " + to_string_dec_uint(data->ui_config.hide_clock()));
    pmem_dump_file.write_line("ui_config clock_with_date: " + to_string_dec_uint(data->ui_config.clock_with_date()));
    pmem_dump_file.write_line("ui_config clkout_enabled: " + to_string_dec_uint(data->ui_config.clkout_enabled()));
    pmem_dump_file.write_line("ui_config stealth_mode: " + to_string_dec_uint(data->ui_config.stealth_mode()));
    pmem_dump_file.write_line("ui_config config_login: " + to_string_dec_uint(data->ui_config.config_login()));
    pmem_dump_file.write_line("ui_config config_splash: " + to_string_dec_uint(data->ui_config.config_splash()));
    // misc_config bits
    pmem_dump_file.write_line("misc_config config_audio_mute: " + to_string_dec_int(config_audio_mute()));
    pmem_dump_file.write_line("misc_config config_speaker_disable: " + to_string_dec_int(config_speaker_disable()));

    // receiver_model
    pmem_dump_file.write_line("[Receiver Model]");
    pmem_dump_file.write_line("target_frequency: " + to_string_dec_uint(receiver_model.target_frequency()));
    pmem_dump_file.write_line("frequency_step: " + to_string_dec_uint(receiver_model.frequency_step()));
    pmem_dump_file.write_line("lna: " + to_string_dec_int(receiver_model.lna()));
    pmem_dump_file.write_line("vga: " + to_string_dec_int(receiver_model.vga()));
    pmem_dump_file.write_line("rf_amp: " + to_string_dec_int(receiver_model.rf_amp()));
    pmem_dump_file.write_line("baseband_bandwidth: " + to_string_dec_uint(receiver_model.baseband_bandwidth()));
    pmem_dump_file.write_line("sampling_rate: " + to_string_dec_uint(receiver_model.sampling_rate()));
    switch (receiver_model.modulation()) {
        case ReceiverModel::Mode::AMAudio:
            pmem_dump_file.write_line("modulation: Mode::AMAudio");
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            pmem_dump_file.write_line("modulation: Mode::NarrowbandFMAudio");
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            pmem_dump_file.write_line("modulation: Mode::WidebandFMAudio");
            break;
        case ReceiverModel::Mode::SpectrumAnalysis:
            pmem_dump_file.write_line("modulation: Mode::SpectrumAnalysis");
            break;
        case ReceiverModel::Mode::Capture:
            pmem_dump_file.write_line("modulation: Mode::Capture");
            break;
        default:
            pmem_dump_file.write_line("modulation: !!unknown mode!!");
            break;
    }
    pmem_dump_file.write_line("headphone_volume.centibel: " + to_string_dec_uint(receiver_model.headphone_volume().centibel()));
    pmem_dump_file.write_line("normalized_headphone_volume: " + to_string_dec_uint(receiver_model.normalized_headphone_volume()));
    pmem_dump_file.write_line("am_configuration: " + to_string_dec_uint(receiver_model.am_configuration()));
    pmem_dump_file.write_line("nbfm_configuration: " + to_string_dec_uint(receiver_model.nbfm_configuration()));
    pmem_dump_file.write_line("wfm_configuration: " + to_string_dec_uint(receiver_model.wfm_configuration()));
    // transmitter_model
    pmem_dump_file.write_line("[Transmitter Model]");
    pmem_dump_file.write_line("target_frequency: " + to_string_dec_uint(transmitter_model.target_frequency()));
    pmem_dump_file.write_line("lna: " + to_string_dec_int(transmitter_model.lna()));
    pmem_dump_file.write_line("vga: " + to_string_dec_int(transmitter_model.vga()));
    pmem_dump_file.write_line("rf_amp: " + to_string_dec_int(transmitter_model.rf_amp()));
    pmem_dump_file.write_line("baseband_bandwidth: " + to_string_dec_uint(transmitter_model.baseband_bandwidth()));
    pmem_dump_file.write_line("sampling_rate: " + to_string_dec_uint(transmitter_model.sampling_rate()));
    pmem_dump_file.write_line("tx_gain: " + to_string_dec_int(transmitter_model.tx_gain()));
    pmem_dump_file.write_line("channel_bandwidth: " + to_string_dec_uint(transmitter_model.channel_bandwidth()));

    painter.draw_string({0, 320 - 16}, ui::Styles::green, filename.filename().string() + " DUMPED !");

    return true;
}

} /* namespace persistent_memory */
} /* namespace portapack */
