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

#ifndef __DATABASE_H__
#define __DATABASE_H__


#include <cstddef>
#include <cstdint>
#include <string>

#include "file.hpp"



namespace std {
class database {



public:

#define DATABASE_RECORD_FOUND	 	 0		// record found in database
#define DATABASE_NOT_FOUND		-1		// database not found / could not be opened
#define DATABASE_RECORD_NOT_FOUND	-2		// record could not be found in database

	struct MidDBRecord {
		char 	country[32];			// country name
	};

	int retrieve_mid_record(MidDBRecord* record, std::string search_term);

	struct AirlinesDBRecord {
		char 	airline[32];			// airline name
		char 	country[32];			// country name
	};

	int retrieve_airline_record(AirlinesDBRecord* record, std::string search_term);

	struct AircraftDBRecord { 
		char 	aircraft_registration[9];	// aircraft registration
		char 	aircraft_manufacturer[33];	// aircraft manufacturer
		char 	aircraft_model[33];		// aircraft model
		char	icao_type[5];			// ICAO type descripton or when unavailable ICAO type designator
		char 	aircraft_owner[33];		// aircraft owner
		char 	aircraft_operator[33];		// aircraft operator

	};

	int retrieve_aircraft_record(AircraftDBRecord* record, std::string search_term);

private:
	string 	file_path = "";				// path inclusing filename
	int  	index_item_length = 0;			// length of index item
	int  	record_length = 0;			// length of record

	File 	db_file { };
	int 	number_of_records = 0;
	int 	position = 0;

	char 	file_buffer[32] { 0 };
	bool 	found = false;

	int 	result = 0;

	int retrieve_record(std::string file_path, int index_item_length, int record_length, void* record, std::string search_term);


};
} // namespace std



#endif/*__DATABASE_H__*/
