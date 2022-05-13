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
#include <cstring>
#include <algorithm>

namespace std {

int app_settings::load(std::string application, AppSettings* settings) {

	if (portapack::persistent_memory::load_app_settings()) {
		file_path = folder+"/"+application+".ini";

		auto error = settings_file.open(file_path);
		if (!error.is_valid()) {
			auto error = settings_file.read(file_content, std::min((int)settings_file.size(), MAX_FILE_CONTENT_SIZE));

			settings->baseband_bandwidth=std::app_settings::read_long_long(file_content, "baseband_bandwidth=");
			settings->channel_bandwidth=std::app_settings::read_long_long(file_content, "channel_bandwidth=");
			settings->lna=std::app_settings::read_long_long(file_content, "lna=");
			settings->modulation=std::app_settings::read_long_long(file_content, "modulation=");
			settings->rx_amp=std::app_settings::read_long_long(file_content, "rx_amp=");
			settings->rx_frequency=std::app_settings::read_long_long(file_content, "rx_frequency=");
			settings->sampling_rate=std::app_settings::read_long_long(file_content, "sampling_rate=");
			settings->vga=std::app_settings::read_long_long(file_content, "vga=");
			settings->tx_amp=std::app_settings::read_long_long(file_content, "tx_amp=");
			settings->tx_frequency=std::app_settings::read_long_long(file_content, "tx_frequency=");
			settings->tx_gain=std::app_settings::read_long_long(file_content, "tx_gain=");
			rc = SETTINGS_OK;
		}
		else rc = SETTINGS_UNABLE_TO_LOAD;
	}
	else rc = SETTINGS_DISABLED;
	return(rc);
}

int app_settings::save(std::string application, AppSettings* settings) {

	if (portapack::persistent_memory::save_app_settings()) {        
		file_path = folder+"/"+application+".ini";
		make_new_directory(folder);

		auto error = settings_file.create(file_path);
		if (!error.is_valid()) {
			// Save common setting
			settings_file.write_line("baseband_bandwidth="+to_string_dec_uint(portapack::receiver_model.baseband_bandwidth()));
			settings_file.write_line("channel_bandwidth="+to_string_dec_uint(portapack::transmitter_model.channel_bandwidth()));
			settings_file.write_line("lna="+to_string_dec_uint(portapack::receiver_model.lna()));
			settings_file.write_line("rx_amp="+to_string_dec_uint(portapack::receiver_model.rf_amp()));
			settings_file.write_line("sampling_rate="+to_string_dec_uint(portapack::receiver_model.sampling_rate()));
			settings_file.write_line("tx_amp="+to_string_dec_uint(portapack::transmitter_model.rf_amp()));
			settings_file.write_line("tx_gain="+to_string_dec_uint(portapack::transmitter_model.tx_gain()));
			settings_file.write_line("vga="+to_string_dec_uint(portapack::receiver_model.vga()));
			// Save other settings from struct
			settings_file.write_line("rx_frequency="+to_string_dec_uint(settings->rx_frequency));
			settings_file.write_line("tx_frequency="+to_string_dec_uint(settings->tx_frequency));

			rc = SETTINGS_OK;
		}
		else rc = SETTINGS_UNABLE_TO_SAVE;
	}
	else rc = SETTINGS_DISABLED;
	return(rc);
}


long long int app_settings::read_long_long(char* file_content, const char* setting_text) {
	auto position = strstr(file_content, (char *)setting_text);
	if (position) {
		position += strlen((char *)setting_text);
		setting_value = strtoll(position, nullptr, 10);
	}
	return(setting_value);
}


} /* namespace std */
