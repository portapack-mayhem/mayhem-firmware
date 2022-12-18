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

#ifndef __APRS_PACKET_H__
#define __APRS_PACKET_H__

#include <cstdint>
#include <cstddef>
#include <cctype>

#include "baseband.hpp"

namespace aprs {

const int APRS_MIN_LENGTH = 18; //14 bytes address, control byte and pid. 2 CRC.

struct aprs_pos{
	float latitude;
	float longitude;	
	uint8_t symbol_code;
	uint8_t  sym_table_id;
};

enum ADDRESS_TYPE {
	SOURCE,
	DESTINATION,
	REPEATER
};

class APRSPacket {
public:
	void set_timestamp(const Timestamp& value) {
		timestamp_ = value;
	}
	
	Timestamp timestamp() const {
		return timestamp_;
	}

	void set(const size_t index, const uint8_t data) {		
		payload[index] = data;

		if(index + 1 > payload_size){
			payload_size = index + 1;
		}
	}

	uint32_t operator[](const size_t index) const {
		return payload[index];
	}
	

	uint8_t size() const {
		return payload_size;
	}

	void set_valid_checksum(const bool valid) {
		valid_checksum = valid;
	}

	bool is_valid_checksum() const {
		return valid_checksum;
	}

	uint64_t get_source(){
		uint64_t source = 0x0;

		for(uint8_t i = SOURCE_START; i < SOURCE_START + ADDRESS_SIZE; i++){
			source |= ( ((uint64_t)payload[i]) << ((i - SOURCE_START) * 8));
		}

		return source;
	}

	std::string get_source_formatted(){
		parse_address(SOURCE_START, SOURCE);
		return std::string(address_buffer);
	}

	std::string get_destination_formatted(){
		parse_address(DESTINATION_START, DESTINATION);
		return std::string(address_buffer);
	}

	std::string get_digipeaters_formatted(){
		uint8_t position = DIGIPEATER_START;
		bool has_more = parse_address(SOURCE_START, REPEATER);

		std::string repeaters = "";
		while(has_more){			
			has_more = parse_address(position, REPEATER);	
			repeaters += std::string(address_buffer);

			position += ADDRESS_SIZE;

			if(has_more){			
				repeaters += ">";
			}		
		}

		return repeaters;
	}

	uint8_t get_number_of_digipeaters(){
		uint8_t position = DIGIPEATER_START;		
		bool has_more = parse_address(SOURCE_START, REPEATER);
		uint8_t repeaters = 0;
		while(has_more){			
			has_more = parse_address(position, REPEATER);	
			position += ADDRESS_SIZE;
			repeaters++;	
		}

		return repeaters;
	}

	uint8_t get_information_start_index(){
		return DIGIPEATER_START + (get_number_of_digipeaters() * ADDRESS_SIZE) + 2;
	}

	std::string get_information_text_formatted(){
		std::string information_text = "";
		for(uint8_t i = get_information_start_index(); i < payload_size - 2; i++){
			information_text += payload[i];
		}

		return information_text;
	}

	std::string get_stream_text(){
		std::string stream = get_source_formatted() + ">" + get_destination_formatted() + ";" + get_digipeaters_formatted() + ";" + get_information_text_formatted();

		return stream;
	}

	char get_data_type_identifier(){
		char ident = '\0';
		for(uint8_t i = get_information_start_index(); i < payload_size - 2; i++){
			ident = payload[i];
			break;
		}
		return ident;
	}

	bool has_position(){		
		char ident = get_data_type_identifier();

		return 
			ident == '!' || 
			ident == '=' || 
			ident == '/' || 
			ident == '@' || 
			ident == ';' || 
			ident == '`' ||
			ident == '\''||
			ident == 0x1d||
			ident == 0x1c;
	}

	aprs_pos get_position(){
		aprs::aprs_pos pos;

		char ident = get_data_type_identifier();
		std::string info_text = get_information_text_formatted();

		std::string lat_str, lng_str;
		char first;
		//bool supports_compression = true;
		bool is_mic_e_format = false;
		std::string::size_type start;

		switch(ident){
			case '/':
			case '@':
				start = 8;
				break;
			case '=':
			case '!':
				start = 1;
				break;
			case ';':			
				start = 18;
				//supports_compression = false;
				break;
			case '`':
			case '\'':
			case 0x1c:
			case 0x1d:
				is_mic_e_format = true;
				break;
			default:
				return pos;
		}		
		if(is_mic_e_format){
			parse_mic_e_format(pos);
		}
		else {			
			if(start < info_text.size()){
				first = info_text.at(start);

				if(std::isdigit(first)){
					if(start + 18 < info_text.size()){
						lat_str = info_text.substr(start, 8);
						pos.sym_table_id = info_text.at(start + 8);
						lng_str = info_text.substr(start + 9, 9);
						pos.symbol_code = info_text.at(start + 18);

						pos.latitude = parse_lat_str(lat_str);
						pos.longitude = parse_lng_str(lng_str);
					}
					
				}
				else {
					if(start + 9 < info_text.size()){
						pos.sym_table_id = info_text.at(start);
						lat_str = info_text.substr(start + 1, 4);
						lng_str = info_text.substr(start + 5, 4);
						pos.symbol_code = info_text.at(start + 9);

						pos.latitude = parse_lat_str_cmp(lat_str);
						pos.longitude = parse_lng_str_cmp(lng_str);	
					}		
				}
			}			
		}		

		return pos;
	}

	void clear() {
		payload_size = 0;
	}

private:
	const uint8_t DIGIPEATER_START = 14;
	const uint8_t SOURCE_START = 7;
	const uint8_t DESTINATION_START = 0;
	const uint8_t ADDRESS_SIZE = 7;

	bool valid_checksum = false;
	uint8_t payload[256];
	char address_buffer[15];
	uint8_t payload_size = 0 ;
	Timestamp timestamp_ { };

	float parse_lat_str_cmp(const std::string& lat_str){
		return 90.0 - ( (lat_str.at(0) - 33) * (91*91*91) + (lat_str.at(1) - 33) * (91*91) + (lat_str.at(2) - 33) * 91 + (lat_str.at(3)) ) / 380926.0;		
	}

	float parse_lng_str_cmp(const std::string& lng_str){
		return -180.0 + ( (lng_str.at(0) - 33) * (91*91*91) + (lng_str.at(1) - 33) * (91*91) + (lng_str.at(2) - 33) * 91 + (lng_str.at(3)) ) / 190463.0;		
	} 

	uint8_t parse_digits(const std::string& str){
		if(str.at(0) == ' '){
			return 0;
		}
		uint8_t end = str.find_last_not_of(' ') + 1;
		std::string sub = str.substr(0, end);

		if(!is_digits(sub)){
			return 0;
		}
		else {
			return std::stoi(sub);
		}
	}

	bool is_digits(const std::string& str){
		return str.find_last_not_of("0123456789") == std::string::npos;
	}

	float parse_lat_str(const std::string& lat_str){
		float lat = 0.0;

		std::string str_lat_deg = lat_str.substr(0, 2);
		std::string str_lat_min = lat_str.substr(2, 2);
		std::string str_lat_hund = lat_str.substr(5, 2);
		std::string dir = lat_str.substr(7, 1);

		uint8_t lat_deg = parse_digits(str_lat_deg);
		uint8_t lat_min = parse_digits(str_lat_min);
		uint8_t lat_hund = parse_digits(str_lat_hund);

		lat += lat_deg;
		lat += (lat_min + (lat_hund/ 100.0))/60.0;

		if(dir.c_str()[0] == 'S'){
			lat = -lat;
		}

		return lat;

	}

	float parse_lng_str(std::string& lng_str){
		float lng = 0.0;

		std::string str_lng_deg = lng_str.substr(0, 3);
		std::string str_lng_min = lng_str.substr(3, 2);
		std::string str_lng_hund = lng_str.substr(6, 2);
		std::string dir = lng_str.substr(8, 1);

		uint8_t lng_deg = parse_digits(str_lng_deg);
		uint8_t lng_min = parse_digits(str_lng_min);
		uint8_t lng_hund = parse_digits(str_lng_hund);

		lng += lng_deg;
		lng += (lng_min + (lng_hund/ 100.0))/60.0;

		if(dir.c_str()[0] == 'W'){
			lng = -lng;
		}

		return lng;

	}

	void parse_mic_e_format(aprs::aprs_pos& pos){
		std::string lat_str = "";
		std::string lng_str = "";

		bool is_north = false;
		bool is_west = false;
		uint8_t lng_offset = 0;
		for(uint8_t i = DESTINATION_START; i < DESTINATION_START + ADDRESS_SIZE - 1; i++){
			uint8_t ascii = payload[i] >> 1;

			lat_str += get_mic_e_lat_digit(ascii);

			if(i - DESTINATION_START == 3){
				lat_str += ".";
			}
			if(i - DESTINATION_START == 3){
				is_north = is_mic_e_lat_N(ascii);
			}
			if(i - DESTINATION_START == 4){
				lng_offset = get_mic_e_lng_offset(ascii);
			}
			if(i - DESTINATION_START == 5){
				is_west = is_mic_e_lng_W(ascii);
			}
		}
		if(is_north){
			lat_str += "N";
		}
		else {
			lat_str += "S";
		}

		pos.latitude = parse_lat_str(lat_str);	

		float lng = 0.0;
		uint8_t information_start = get_information_start_index() + 1;
		for(uint8_t i = information_start; i < information_start + 3 && i < payload_size - 2; i++){
			uint8_t ascii = payload[i];
			
			if(i - information_start == 0){ //deg
				ascii -=28;
				ascii += lng_offset;

				if(ascii >= 180 && ascii <= 189){
					ascii -= 80;
				}
				else if (ascii >= 190 && ascii <= 199){
					ascii -= 190;
				}

				lng += ascii;
			}	
			else if(i - information_start == 1){ //min
				ascii -= 28;
				if(ascii >= 60){
					ascii -= 60;
				}
				lng += ascii/60.0;
			}	
			else if(i - information_start == 2){ //hundredth minutes
				ascii -= 28;

				lng += (ascii/100.0)/60.0;
			}	
		}

		if(is_west){
			lng = -lng;
		}

		pos.longitude = lng;

	}

	uint8_t get_mic_e_lat_digit(uint8_t ascii){
		if(ascii >= '0' && ascii <= '9'){
			return ascii;
		}
		if(ascii >= 'A' && ascii <= 'J'){
			return ascii - 17;
		}
		if(ascii >= 'P' && ascii <='Y'){
			return ascii - 32;
		}
		if(ascii == 'K' || ascii == 'L' || ascii == 'Z'){
			return ' ';
		}

		return '\0';
	}

	bool is_mic_e_lat_N(uint8_t ascii){
		if(ascii >= 'P' && ascii <='Z'){
			return true;
		}		
		return false; //not technical definition, but the other case is invalid
	}

	bool is_mic_e_lng_W(uint8_t ascii){
		if(ascii >= 'P' && ascii <='Z'){
			return true;
		}		
		return false; //not technical definition, but the other case is invalid
	}

	uint8_t get_mic_e_lng_offset(uint8_t ascii){
		if(ascii >= 'P' && ascii <='Z'){
			return 100;
		}		
		return 0; //not technical definition, but the other case is invalid
	}

	bool parse_address(uint8_t start, ADDRESS_TYPE address_type){
		uint8_t byte = 0;
		uint8_t has_more = false;
		uint8_t ssid = 0;
		uint8_t buffer_index = 0;

		for(uint8_t i = start; i < start + ADDRESS_SIZE && i < payload_size - 2; i++){
			byte = payload[i];

			if(i - start == 6){
				 has_more = (byte & 0x1) == 0;
				 ssid = (byte >> 1) & 0x0F;

				 if(ssid != 0 || address_type == REPEATER){
				 	address_buffer[buffer_index++] = '-';				 	

				 	if(ssid < 10){
				 		address_buffer[buffer_index++] = '0' + ssid;				 		
				 		address_buffer[buffer_index++] = '\0';
				 	}
				 	else {
				 		address_buffer[buffer_index++] = '1';
				 		address_buffer[buffer_index++] = '0' + ssid - 10;
				 		address_buffer[buffer_index++] = '\0';
				 	}
				 }
				 else {
					 address_buffer[buffer_index++] = '\0';
				 }
			}
			else {
				byte >>= 1;

				if(byte != ' '){
					address_buffer[buffer_index++] = byte;		
				}			
			}
		}

		return has_more;
	}
};

} /* namespace aprs */

#endif/*__APRS_PACKET_H__*/
