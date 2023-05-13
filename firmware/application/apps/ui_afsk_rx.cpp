/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_afsk_rx.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;

void AFSKLogger::log_raw_data(const std::string& data) {
	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	
	log_file.write_entry(datetime, data);
}

namespace ui {

void AFSKRxView::focus() {
	field_frequency.focus();
}

void AFSKRxView::update_freq(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

AFSKRxView::AFSKRxView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_afsk_rx);
	
	add_children({
		&rssi,
		&channel,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&field_frequency,
		&check_log,
		&text_debug,
		&button_modem_setup,
		&console
	});
	
	// load app settings
	auto rc = settings.load("rx_afsk", &app_settings);
	if(rc == SETTINGS_OK) {
		field_lna.set_value(app_settings.lna);
		field_vga.set_value(app_settings.vga);
		field_rf_amp.set_value(app_settings.rx_amp);
	}

	// Auto-configure modem for LCR RX (will be removed later)
	update_freq(467225500);	// 462713300
	auto def_bell202 = &modem_defs[0];
	persistent_memory::set_modem_baudrate(def_bell202->baudrate);
	serial_format_t serial_format;
	serial_format.data_bits = 7;
	serial_format.parity = EVEN;
	serial_format.stop_bits = 1;
	serial_format.bit_order = LSB_FIRST;
	persistent_memory::set_serial_format(serial_format);
	
	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(100);
	field_frequency.on_change = [this](rf::Frequency f) {
		update_freq(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			update_freq(f);
			field_frequency.set_value(f);
		};
	};

	check_log.set_value(logging);
	check_log.on_select = [this](Checkbox&, bool v) {
		logging = v;
	};

	button_modem_setup.on_select = [&nav](Button&) {
		nav.push<ModemSetupView>();
	};
	
	logger = std::make_unique<AFSKLogger>();
	if (logger)
		logger->append(LOG_ROOT_DIR "/AFSK.TXT");
	
	// Auto-configure modem for LCR RX (will be removed later)
	baseband::set_afsk(persistent_memory::modem_baudrate(), 8, 0, false);
	
	audio::set_rate(audio::Rate::Hz_24000);
	audio::output::start();
	
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
	receiver_model.enable();
}

void AFSKRxView::on_data(uint32_t value, bool is_data) {
	std::string str_console = "\x1B";
	std::string str_byte = "";
	
	if (is_data) {
		// Colorize differently after message splits
		str_console += (char)((console_color & 3) + 9);
		
		//value = deframe_word(value);
		
		value &= 0xFF;											// ABCDEFGH
		value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);	// EFGHABCD
		value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);	// GHEFCDAB
		value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);	// HGFEDCBA
		value &= 0x7F;											// Ignore parity, which is the MSB now
		
		if ((value >= 32) && (value < 127)) {
			str_console += (char)value;							// Printable
			str_byte += (char)value;
		} else {
			str_console += "[" + to_string_hex(value, 2) + "]";	// Not printable
			str_byte += "[" + to_string_hex(value, 2) + "]";
		}
		
		//str_byte = to_string_bin(value & 0xFF, 8) + "  ";
		
		console.write(str_console);
		
		if (logger && logging) str_log += str_byte;
		
		if ((value != 0x7F) && (prev_value == 0x7F)) {
			// Message split
			console.writeln("");
			console_color++;
			
			if (logger && logging) {
				logger->log_raw_data(str_log);
				str_log = "";
			}
		}
		prev_value = value;
	}
	else {
		// Baudrate estimation
		text_debug.set("Baudrate estimation: ~" + to_string_dec_uint(value));
	}
}

AFSKRxView::~AFSKRxView() {

	// save app settings
	app_settings.rx_frequency = field_frequency.value();
	settings.save("rx_afsk", &app_settings);

	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

} /* namespace ui */
