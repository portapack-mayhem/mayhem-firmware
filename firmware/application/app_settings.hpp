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

#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__


#include <cstddef>
#include <cstdint>
#include <string>

#include "file.hpp"
#include "string_format.hpp"


namespace std {
class app_settings {



public:

#define SETTINGS_OK	 	 	0		// settings found
#define SETTINGS_UNABLE_TO_LOAD		-1		// settings (file) not found
#define SETTINGS_UNABLE_TO_SAVE		-2		// unable to save settings
#define SETTINGS_DISABLED		-3		// load/save settings disabled in settings


	// store settings that can't be set directly, but have to be stored in app	
	struct AppSettings {
		uint32_t 	baseband_bandwidth;
		uint8_t 	lna;
		uint8_t 	rx_amp;
		uint32_t 	rx_frequency;
		uint32_t 	sampling_rate;
		uint8_t 	vga;
	};

	int load(std::string application, AppSettings* settings);
	int save(std::string application, AppSettings* settings);


private:


	char 		file_content[257] = {};	
	std::string 	file_path = "";
	std::string 	folder = "SETTINGS";
	int		rc = SETTINGS_OK;
	File 		settings_file { };	


}; // class app_settings
} // namespace std



#endif/*__APP_SETTINGS_H__*/
