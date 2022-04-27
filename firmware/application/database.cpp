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

#include "database.hpp"
#include "file.hpp"
#include <cstring>

namespace std {

int database::retrieve_mid_record(MidDBRecord* record, std::string search_term){
        
	file_path = "AIS/mids.db";
	index_item_length = 4;
	record_length = 32;

	result = std::database::retrieve_record(file_path, index_item_length, record_length, record, search_term);

	return(result);
}

int database::retrieve_airline_record(AirlinesDBRecord* record, std::string search_term){
        
	file_path = "ADSB/airlines.db";
	index_item_length = 4;
	record_length = 64;

	result = std::database::retrieve_record(file_path, index_item_length, record_length, record, search_term);

	return(result);
}

int database::retrieve_aircraft_record(AircraftDBRecord* record, std::string search_term){

	file_path = "ADSB/icao24.db";
	index_item_length = 7;
	record_length = 146;

	result = std::database::retrieve_record(file_path, index_item_length, record_length, record, search_term);

	return(result);
}



int database::retrieve_record(std::string file_path, int index_item_length, int record_length, void* record, std::string search_term)
{

	auto result = db_file.open(file_path);
	if (!result.is_valid()) {
		number_of_records = (db_file.size() / (index_item_length + record_length)); // determine number of records in file
  		// binary search tree
    		int first = 0,         				// First search element       
    		last = number_of_records - 1,        	 	// Last search element       
    		middle,                				// Mid point of search       
    		position = -1;         				// Position of search value   

    		while (!found && first <= last) {  
        		middle = (first + last) / 2;     	// Calculate mid point      
        		db_file.seek(middle * index_item_length); 
			db_file.read(file_buffer, search_term.length());
			if (file_buffer == search_term) {     	// If value is found at mid        
                		found = true;         
                		position = middle;      
        		}      
        		else if (file_buffer > search_term)  	// If value is in lower half         
            			last = middle - 1;      
        		else         
            			first = middle + 1;          	// If value is in upper half   
    		}

		if(found == true) {

			db_file.seek((number_of_records * index_item_length) + (position * record_length)); // seek starting after index
			db_file.read(record, record_length);
			return(DATABASE_RECORD_FOUND);
		}
		else {
			return(DATABASE_RECORD_NOT_FOUND);
		}

	}
	else return(DATABASE_NOT_FOUND);

}


} /* namespace std */
