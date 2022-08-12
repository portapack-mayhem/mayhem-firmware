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

#include "portapack.hpp"
#include "hal.h"

#include "utility.hpp"

#include "memory_map.hpp"

#include "crc.hpp"

#include <algorithm>
#include <utility>

namespace portapack {
namespace persistent_memory {

constexpr rf::Frequency tuned_frequency_reset_value { 100000000 };

using ppb_range_t = range_t<ppb_t>;
constexpr ppb_range_t ppb_range { -99000, 99000 };
constexpr ppb_t ppb_reset_value { 0 };

using tone_mix_range_t = range_t<int32_t>;
constexpr tone_mix_range_t tone_mix_range { 10, 99 };
constexpr int32_t tone_mix_reset_value { 20 };

using afsk_freq_range_t = range_t<int32_t>;
constexpr afsk_freq_range_t afsk_freq_range { 1, 4000 };
constexpr int32_t afsk_mark_reset_value { 1200 };
constexpr int32_t afsk_space_reset_value { 2200 };

using modem_baudrate_range_t = range_t<int32_t>;
constexpr modem_baudrate_range_t modem_baudrate_range { 50, 9600 };
constexpr int32_t modem_baudrate_reset_value { 1200 };

/*using modem_bw_range_t = range_t<int32_t>;
constexpr modem_bw_range_t modem_bw_range { 1000, 50000 };
constexpr int32_t modem_bw_reset_value { 15000 };*/

using modem_repeat_range_t = range_t<int32_t>;
constexpr modem_repeat_range_t modem_repeat_range { 1, 99 };
constexpr int32_t modem_repeat_reset_value { 5 };

using clkout_freq_range_t = range_t<uint32_t>;
constexpr clkout_freq_range_t clkout_freq_range { 10, 60000 };
constexpr uint32_t clkout_freq_reset_value { 10000 };

enum data_structure_version_enum : uint32_t {
	VERSION_CURRENT = 0x10000002,
};

static const uint32_t TOUCH_CALIBRATION_MAGIC = 0x074af82f;

struct ui_config_t {
private:
	enum bits_t {
		BacklightTimeoutLSB    =  0,
		BacklightTimeoutEnable =  3,
		ClkoutFreqLSB          =  4,
		ShowGUIReturnIcon      = 20,
		LoadAppSettings        = 21,
		SaveAppSettings        = 22,
		ShowBiggerQRCode       = 23,
		DisableTouchscreen     = 24,
		HideClock              = 25,
		ClockWithDate          = 26,
		ClkOutEnabled          = 27,
		ConfigSpeaker          = 28,
		StealthMode            = 29,
		ConfigLogin            = 30,
		ConfigSplash           = 31,
	};

	enum bits_mask_t : uint32_t {
		BacklightTimeoutMask = ((1 <<  3) - 1) << bits_t::BacklightTimeoutLSB,
		ClkoutFreqMask       = ((1 << 16) - 1) << bits_t::ClkoutFreqLSB,
	};

	uint32_t values;

	constexpr bool bit_read(const bits_t n) const {
		return ((values >> n) & 1) != 0;
	}

	constexpr void bit_write(const bits_t n, const bool v) {
		if(bit_read(n) != v) {
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
		values = (values & ~bits_mask_t::BacklightTimeoutMask)
			| ((new_value.timeout_enum() << bits_t::BacklightTimeoutLSB) & bits_mask_t::BacklightTimeoutMask);
		bit_write(bits_t::BacklightTimeoutEnable, new_value.timeout_enabled());
	}

	constexpr uint32_t clkout_freq() {
		uint32_t freq = (values & bits_mask_t::ClkoutFreqMask) >> bits_t::ClkoutFreqLSB;
		if(freq < clkout_freq_range.minimum || freq > clkout_freq_range.maximum) {
			values = (values & ~bits_mask_t::ClkoutFreqMask) | (clkout_freq_reset_value << bits_t::ClkoutFreqLSB);
			return clkout_freq_reset_value;
		}
		else {
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

	constexpr bool show_gui_return_icon() const { // add return icon in touchscreen menue
		return bit_read(bits_t::ShowGUIReturnIcon);
	}

	constexpr void set_gui_return_icon(bool v) {
		bit_write(bits_t::ShowGUIReturnIcon, v);
	}

	constexpr bool load_app_settings() const { // load (last saved) app settings on startup of app
		return bit_read(bits_t::LoadAppSettings);
	}

	constexpr void set_load_app_settings(bool v) {
		bit_write(bits_t::LoadAppSettings, v);
	}

	constexpr bool save_app_settings() const { // save app settings when closing app
		return bit_read(bits_t::SaveAppSettings);
	}

	constexpr void set_save_app_settings(bool v) {
		bit_write(bits_t::SaveAppSettings, v);
	}

	constexpr bool show_bigger_qr_code() const { // show bigger QR code
		return bit_read(bits_t::ShowBiggerQRCode);
	}

	constexpr void set_show_bigger_qr_code(bool v) {
		bit_write(bits_t::ShowBiggerQRCode, v);
	}

	constexpr bool disable_touchscreen() const { // Option to disable touch screen
		return bit_read(bits_t::DisableTouchscreen);
	}

	constexpr void set_disable_touchscreen(bool v) {
		bit_write(bits_t::DisableTouchscreen, v);
	}

	constexpr bool hide_clock() const { // clock hidden from main menu
		return bit_read(bits_t::HideClock);
	}

	constexpr void set_clock_hidden(bool v) {
		bit_write(bits_t::HideClock, v);
	}

	constexpr bool clock_with_date() const { // show clock with date, if not hidden
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

	constexpr bool config_speaker() const {
		return bit_read(bits_t::ConfigSpeaker);
	}

	constexpr void set_config_speaker(bool v) {
		bit_write(bits_t::ConfigSpeaker, v);
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

	constexpr ui_config_t() :
		values(
		      (1 << ConfigSplash)
			| (1 << ConfigSpeaker)
			| (clkout_freq_reset_value << ClkoutFreqLSB)
			| (7 << BacklightTimeoutLSB)
		)
	{
	}
};

/* struct must pack the same way on M4 and M0 cores. */
struct data_t {
	data_structure_version_enum structure_version;
	int64_t tuned_frequency;
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

	constexpr data_t() :
		structure_version(data_structure_version_enum::VERSION_CURRENT),
		tuned_frequency(tuned_frequency_reset_value),
		correction_ppb(ppb_reset_value),
		touch_calibration_magic(TOUCH_CALIBRATION_MAGIC),
		touch_calibration(touch::Calibration()),

		modem_def_index(0),			// TODO: Unused?
		serial_format(),
		modem_bw(15000),			// TODO: Unused?
		afsk_mark_freq(afsk_mark_reset_value),
		afsk_space_freq(afsk_space_reset_value),
		modem_baudrate(modem_baudrate_reset_value),
		modem_repeat(modem_repeat_reset_value),

		playdead_magic(),			// TODO: Unused?
		playing_dead(),				// TODO: Unused?
		playdead_sequence(),		// TODO: Unused?

		ui_config(),

		pocsag_last_address(0),		// TODO: A better default?
		pocsag_ignore_address(0),	// TODO: A better default?

		tone_mix(tone_mix_reset_value),

		hardware_config(0)
	{
	}
};

struct backup_ram_t {
private:
	uint32_t regfile[63];
	uint32_t check_value;

	static void copy(const backup_ram_t& src, backup_ram_t& dst) {
		for(size_t i=0; i<63; i++) {
			dst.regfile[i] = src.regfile[i];
		}
		dst.check_value = src.check_value;
	}

	static void copy_from_data_t(const data_t& src, backup_ram_t& dst) {
		const uint32_t* const src_words = (uint32_t*)&src;
		const size_t word_count = (sizeof(data_t) + 3) / 4;
		for(size_t i=0; i<63; i++) {
			if(i<word_count) {
				dst.regfile[i] = src_words[i];
			} else {
				dst.regfile[i] = 0;
			}
		}
	}

	uint32_t compute_check_value() {
		CRC<32> crc { 0x04c11db7, 0xffffffff, 0xffffffff };
		for(size_t i=0; i<63; i++) {
			const auto word = regfile[i];
			crc.process_byte((word >>  0) & 0xff);
			crc.process_byte((word >>  8) & 0xff);
			crc.process_byte((word >> 16) & 0xff);
			crc.process_byte((word >> 24) & 0xff);
		}
		return crc.checksum();
	}

public:
	/* default constructor */
	backup_ram_t() :
		check_value(0)
	{
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
static data_t* const data = reinterpret_cast<data_t*>(&cached_backup_ram);

namespace cache {

void defaults() {
	cached_backup_ram = backup_ram_t();
}

void init() {
	if(backup_ram->is_valid()) {
		// Copy valid persistent data into cache.
		cached_backup_ram = *backup_ram;

		// Check that structure data we copied into cache is the expected
		// version. If not, initialize cache to defaults.
		if(data->structure_version != data_structure_version_enum::VERSION_CURRENT) {
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

rf::Frequency tuned_frequency() {
	rf::tuning_range.reset_if_outside(data->tuned_frequency, tuned_frequency_reset_value);
	return data->tuned_frequency;
}

void set_tuned_frequency(const rf::Frequency new_value) {
	data->tuned_frequency = rf::tuning_range.clip(new_value);
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
	if( data->touch_calibration_magic != TOUCH_CALIBRATION_MAGIC ) {
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

bool show_gui_return_icon() { // add return icon in touchscreen menue
	return data->ui_config.show_gui_return_icon();
}

bool load_app_settings() { // load (last saved) app settings on startup of app
	return data->ui_config.load_app_settings();
}

bool save_app_settings() { // save app settings when closing app
	return data->ui_config.save_app_settings();
}
  
bool show_bigger_qr_code() { // show bigger QR code
	return data->ui_config.show_bigger_qr_code();
}

bool disable_touchscreen() { // Option to disable touch screen
	return data->ui_config.disable_touchscreen();
}

bool hide_clock() { // clock hidden from main menu
	return data->ui_config.hide_clock();
}

bool clock_with_date() { // show clock with date, if not hidden
	return data->ui_config.clock_with_date();
}

bool clkout_enabled() {
	return data->ui_config.clkout_enabled();
}

bool config_speaker() {
	return data->ui_config.config_speaker();
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

void set_config_speaker(bool v) {
	data->ui_config.set_config_speaker(v);
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


} /* namespace persistent_memory */
} /* namespace portapack */
