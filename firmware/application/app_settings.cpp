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

namespace std {

int app_settings::load(std::string application, AppSettings* settings){

	if (portapack::persistent_memory::load_app_settings()) {
		file_path = folder+"/"+application+".ini";

		auto error = settings_file.open(file_path);
		if (!error.is_valid()) {
			auto error = settings_file.read(file_content, 256);
			
			// Retrieve settings
			auto position1 = strstr(file_content, "baseband_bandwidth=");
			if (position1) {
				position1 += 19;
				settings->baseband_bandwidth=strtoll(position1, nullptr, 10);
			}

			auto position2 = strstr(file_content, "rx_frequency=");
			if (position2)  {
				position2 += 13;
				settings->rx_frequency=strtoll(position2, nullptr, 10);
			}

			auto position3 = strstr(file_content, "lna=");
			if (position3) {
				position3 += 4;
				settings->lna=strtoll(position3, nullptr, 10);
			}

			auto position4 = strstr(file_content, "rx_amp=");
			if (position4) {
				position4 += 7;
				settings->rx_amp=strtoll(position4, nullptr, 10);
			}

			auto position5 = strstr(file_content, "sampling_rate=");
			if (position5) {
				position5 += 13;
				settings->sampling_rate=strtoll(position5, nullptr, 10);
			}


			auto position6 = strstr(file_content, "vga=");
			if (position6) {
				position6 += 4;
				settings->vga=strtoll(position6, nullptr, 10);
			} 

			rc = SETTINGS_OK;
		}
		else rc = SETTINGS_UNABLE_TO_LOAD;
	}
	else rc = SETTINGS_DISABLED;
	return(rc);
}

int app_settings::save(std::string application, AppSettings* settings){

	if (portapack::persistent_memory::save_app_settings()) {        
		file_path = folder+"/"+application+".ini";
		make_new_directory(folder);

		auto error = settings_file.create(file_path);
		if (!error.is_valid()) {
			// Save common setting
			settings_file.write_line("baseband_bandwidth="+to_string_dec_uint(portapack::receiver_model.baseband_bandwidth()));
			settings_file.write_line("lna="+to_string_dec_uint(portapack::receiver_model.lna()));
			settings_file.write_line("rx_amp="+to_string_dec_uint(portapack::receiver_model.rf_amp()));
			settings_file.write_line("sampling_rate="+to_string_dec_uint(portapack::receiver_model.sampling_rate()));
			settings_file.write_line("vga="+to_string_dec_uint(portapack::receiver_model.vga()));
			// Save other settings from struct
			settings_file.write_line("rx_frequency="+to_string_dec_uint(settings->rx_frequency));

			rc = SETTINGS_OK;
		}
		else rc = SETTINGS_UNABLE_TO_SAVE;
	}
	else rc = SETTINGS_DISABLED;
	return(rc);
}

} /* namespace std */
