/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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
//  AD V 1.0 7/6/2020
#include "freqman.hpp"
#include <algorithm>

std::vector<std::string> get_freqman_files() {
	std::vector<std::string> file_list;
	
	auto files = scan_root_files(u"FREQMAN", u"*.TXT");
	
	for (auto file : files) {
//		DEBUG (3,"GET freqman " + file.stem().string() );
		file_list.emplace_back(file.stem().string());
	}
	return file_list;
};

bool load_freqman_file(std::string& file_stem, freqman_db &db) {
	File freqman_file;
	size_t length, n = 0, file_position = 0;
	char * pos = nullptr;			
	char * line_start = nullptr;
	char * line_end = nullptr;
	size_t cas  = 20;
	char test_car = 'X';
	char hex_car[2];
	
	std::string description;
	rf::Frequency frequency_a, frequency_b;
	char file_data[257];
	freqman_entry_type type;
	freqman_entry_step step;
//	 DEBUG (3,"IN LOAD FREQMAN ");
	
	db.clear();
	
	auto result = freqman_file.open("FREQMAN/" + file_stem + ".TXT");
	if (result.is_valid()) {	return false; }
	
	while (1) {
		
		freqman_file.seek(file_position);					// Read a 256 bytes block from file
		
		memset(file_data, 0, 257);
		auto read_size = freqman_file.read(file_data, 256);
		if (read_size.is_error())
			return false;	// Read error
		
		// To get rid of NULL caracter
		for  (int i = 0; i < read_size.value(); i++) {
			 if (file_data[i] == 0 ) 
			 { file_data[i] = 219 ; 
			//DEBUG (3,"CARACTERE NULL !!!" );
			 }
		}
		
		file_position += 256;
		
		// Reset line_start to beginning of buffer
		line_start = file_data;
		 
		// Look for complete lines in buffer
		while ((line_end = strstr(line_start, "\n"))) {  
//	DEBUG (3,"deb: "  + file_stem + " L: " + to_string_dec_uint(n+1) );	

			// Read frequency  / frequencies  		
			//Init
			frequency_a = 0;
			frequency_b = 0;
			type = ERROR;										// by default   third type
			cas = 20;   // UNKNOWN (by default)
			pos = 0 ;
			step = STEP_DEF;

			line_start[line_end - line_start] = 0 ;															// Analyze per LINE, not per Buffer 

			// first test
			if (line_end != line_start )  {    																														// test empty line
		
			// second test
			if ( strstr(line_start, "f=") || strstr(line_start, "a=") || strstr(line_start, "b=") || strstr(line_start, "d=") || strstr(line_start, "//") || strstr(line_start, "#") )  {    	// no useful information 

			// Test if special caracters      ( ok is > 31 and < 127 or 13) 

		// DEBUG (3,"deb while F: "  + file_stem + " L: " + to_string_dec_uint(n+1) + " LINE-start :" + line_start  );
			
			int i=0;
			while( line_start[i] != 0 )
			 {
				 test_car = line_start[i];		
				//description = "Caract:" + to_string_dec_uint(i+1) + "<" + test_car + ">" ;
				//DEBUG (3, description  );
				if	(   ((test_car > 31 && test_car < 127) ) || (test_car == 13)     )  		{ i++; continue; }
				else {
					cas = 22 ;
				
			uint32_t d = test_car & 0xf; 	hex_car[1] = (d > 9) ? (d + 55) : (d + 48);				// CONVERT ASCII/HEW
					d = (test_car>> 4) & 0xf; 	hex_car[0] = (d > 9) ? (d + 55) : (d + 48);
					hex_car[2] = 0 ;
	
					description = "P:" + to_string_dec_uint(i+1) + "<" + test_car + "><" + hex_car + ">";
					DEBUG (3,"DESC= " +description ) ;
					type = ERROR ;
					break;
					}
			 }
   
			// third test     (special caracters)
			if (cas == 20) {																																				
				   
			// fourth test Read comment  // OR #
			if (	(!strstr(line_start, "//")) &&  (!strstr(line_start, "#")))	{ 	  																	// Comment line
						
			// Read Frequency  f=, a= or b=
				pos = strstr(line_start, "f=");		//by default
					if (pos) 	{	
								// test one freq   f= 
								pos += 2; 
								frequency_a = strtoll(pos, nullptr, 10);
								cas = 2;
								type = SINGLE;
								if (( frequency_a <= 0) || (frequency_a > 7500000000)) {cas = 12; type = ERROR;} 
								// freq f:    ok
								} 
								else 	{																	// test freq a:  and/or b:
															
										pos = strstr(line_start, "a=");
										if (pos) 	{														// test one freq   a=
													pos += 2;
													frequency_a = strtoll(pos, nullptr, 10);
													cas = 3 ;
													type = RANGE;											// freq a: ok
													if (( frequency_a <= 0) || (frequency_a > 7500000000)) {cas = 13 ; type = ERROR ;}
													}
													else 	
													{
													cas = 7; 												// no freq a:
													}										
									pos = strstr(line_start, "b=");
										if (pos) 	{				// two frequ  a= +  b=
													pos += 2;
													frequency_b = strtoll(pos, nullptr, 10);
													type = RANGE;
													if (( frequency_b <= 0) || (frequency_b > 7500000000)) {cas = 14 ;type = ERROR ;}
														else
														{
													if ( frequency_a >= frequency_b) {cas = 15 ; type = ERROR ;}	

														}													
														
														if (cas == 3) 	{	cas = 4; } 												// freq a: and b:   	
															else {
															if ((cas != 13) && (cas != 14 ) && (cas != 15)) { 	cas = 6; type = ERROR ;	}	}				// freq b: but not a:
																															
													} 
													else  											// no freq b:	
													{	
													if (cas == 3) 	{
																	cas = 7;type = ERROR ;						// freq a: but not b:
																	}			
																	else 	
																	{
																		if (cas != 13) 	{ cas = 10 ;type = ERROR ; }						// not freq a:, not freq b:
																	} 						
													}						
										}
																				
			if ((cas == 2 ) || (cas == 4 ) )  {														// if Freq ok
				// Read description until , or LF
				pos = strstr(line_start, "d=");
					if (pos) 	{
								pos += 2;		
								length = strcspn(pos, ",\x0A");
								if (length <  (size_t)FREQMAN_DESC_MAX_LEN ) 	{										//desc ok
																				description = string(pos, length);
																				}
																				else 	
																				{ 		
																				cas = 9;type = ERROR ;			// descrip too long			
																				}
								if (length == 0 ) 	{		cas = 8;type = ERROR ;		} 							//desc empty
																				
								} 	
								else 	{
										if (cas == 4)	{
														cas = 8;type = ERROR ;						// RANGE without DESCRIPTION
														}    									
														else	{
																description = "---";			// no description pour freq a:
																}							
										}
											}	
// debug
								else 
								{
								description = to_string_dec_uint(cas) + "/" +description ;
								type = ERROR ;								
								}
// Analyze STEP  only for RANGE
			if  (cas == 4 )  {
				pos = strstr(line_start, "s=");
						if (pos) {
							switch (strtoll(pos + 2, nullptr, 10)) 	{
							case 0:		step = STEP_DEF  	;  	break ;
							case 1: 	step = AM_US 		;  	description+= "-" + to_string_dec_uint(step); 	break ;
							case 2:    	step = AM_EUR		;	description+= "-" + to_string_dec_uint(step); 	break ;																	
							case 3: 	step = NFM_1 		;  	description+= "-" + to_string_dec_uint(step); 	break ;
							case 4:  	step = NFM_2		; 	description+= "-" + to_string_dec_uint(step);  	break ;    																
							case 5: 	step = FM_1 		;  	description+= "-" + to_string_dec_uint(step); 	break ;
							case 6: 	step = FM_2   		;  	description+= "-" + to_string_dec_uint(step); 	break ;		
							case 7: 	step = N_1 			;  	description+= "-" + to_string_dec_uint(step); 	break ;
							case 8: 	step = N_2   		;  	description+= "-" + to_string_dec_uint(step); 	break ;	
							case 9: 	step = AIRBAND   	;  	description+= "-" + to_string_dec_uint(step); 	break ;							
							default:	step= ERROR_STEP 	; cas = 16 ;				
																}
								}
							}			
								
						} 	else {cas = 21;			type = COMMENT ; }				  //COMMENT		
					}	else {cas = 22 ;}
				}	else {cas = 0;}
			}	else {cas = 11;type = ERROR;}
// TEST ERRORS
			switch (cas) {
				case 0:		description = "[" + to_string_dec_uint(n + 1) + "]:" + "No Useful Info"  ;  break ;
	//			case 1: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "No Unix EOL (0x0D)"  ;  break ;
				case 2:    	break ;																	// 	freq f
				case 3: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "No Freq B:"  ;  break ;
				case 4:    	break ;    																//     freq a and b
				case 5: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "No F + D"  ;  break ;
				case 6: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "No A: but B:"   ;  break ;								//ok
				case 7: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "No B: but A:"   ;  break ;
				case 8: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "R w/o Description"   ;  break ;
				case 9: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "D too long"   ;  break ;
				case 10: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "No Freq A: & B:"   ;  break ;
				case 11: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "Empty Line"   ;  break ;
				case 12: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "F: out of limit"   ;  frequency_a = 0 ; break ;
				case 13: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "A: out of limit"   ; frequency_a = 0 ; break ;
				case 14: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "B: out of limit"   ;  frequency_b = 0 ;break ;
				case 15: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "Freq A: > B:"   ;   frequency_a = 0  ;  frequency_b = 0 ; break ;
				case 16: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "Step Err"   ;  break ;
				case 20: 	description = "[" + to_string_dec_uint(n + 1) + "]:" + "UNKNOWN"   ;  break ;
				case 21:	description = line_start; break ;		 // comment
				case 22:	description = "[" + to_string_dec_uint(n + 1) + "]:" + "CAR:" + description  ;  break ;				
				default:	description = "[" + to_string_dec_uint(n + 1) + "]:" + "ERROR" ;
						}

		  db.push_back({ frequency_a, frequency_b, description, type, step }); 

			n++;  // next line  in the buffer
		
			if ( n >= FREQMAN_MAX_PER_FILE )  
					{
						description = "[" + to_string_dec_uint(n ) + "]:" +  "Lines >" +  to_string_dec_uint(FREQMAN_MAX_PER_FILE) ;
						type = ERROR ;
						db.push_back({ 0, 0, description, type , step});  			
					//		DEBUG (3,"OUT load freqman too many LINES");			
						return true;
					} 
											
			line_start = line_end + 1; 	 // next line  in the same line_start buffer
			
		if (line_start - file_data >= 256)  { 	break;	  } // end of buffer  
		}
		if (read_size.value() != 256) {break;	}																				// End of file
			
		// Restart at beginning of last incomplete line		
	if (line_start != file_data )    {	file_position -= (file_data + 256 - line_start); }
	}
	return true;
}

bool save_freqman_file(std::string& file_stem, freqman_db &db) {
	File freqman_file;
	std::string item_string;
	std::string description;
	rf::Frequency frequency_a, frequency_b;
	
//	DEBUG (3,"IN SAVE:" + file_stem );   
	
	if (!create_freqman_file(file_stem, freqman_file))
		return false;
	
	for (size_t n = 0; n < db.size(); n++) {
		auto& entry = db[n];

		frequency_a = entry.frequency_a;
//		DEBUG (3,"IN SAVE n:" + to_string_dec_uint(n) + " typ; " +to_string_dec_uint(entry.type) );
	switch (entry.type) 	{
		case 0:		item_string = "f=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
					if (entry.description.size()) {item_string += ",d=" + entry.description;}	 	
					break ;								//SINGLE
		case 1: 	frequency_b = entry.frequency_b;
					item_string = "a=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
					item_string += ",b=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
					if (entry.description.size()) {item_string += ",d=" + entry.description;}
					break ;									//RANGE			
		case 2:    																										// ERROR NO BREAK
		case 3:		item_string = description;	break;																	// COMMENT
		default:	item_string = "BIG ERROR";																				// error				
							} 
//		DEBUG (3,"OUT SAVE<" + item_string + "<");
		freqman_file.write_line_n(item_string);
	}

	return true;
}

bool create_freqman_file(std::string& file_stem, File& freqman_file) {
	
	auto result = freqman_file.create("FREQMAN/" + file_stem + ".TXT");
	if (result.is_valid()) {	return false; }
		{  return true; } 
}

std::string freqman_item_string(freqman_entry &entry, size_t max_length) {
	std::string item_string;

			switch (entry.type) 	{
							case 0:		item_string = to_string_short_freq(entry.frequency_a) + "M:" + entry.description;	;  	break ;			//SINGLE
							case 1: 	item_string = "Range:" + entry.description;		;  	break ;		// RANGE
							case 2:    	item_string = "Err:" + entry.description	;	break ;			// ERROR
							case 3:    	item_string = entry.description ; break ;						// COMMENT		we do nothing 					
							default:	item_string = "BIG ERROR 2"		;	// error				
								} 
//	DEBUG (3,"ITEM STRING: " + item_string );
	if (item_string.size() > max_length)
		return item_string.substr(0, max_length - 3) + "...";
	return item_string;
}


