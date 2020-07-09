/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "log_file.hpp"
#include "file.hpp"

#include "string_format.hpp"

Optional<File::Error> LogFile::write_entry(const rtc::RTC& datetime, const std::string& entry) {
	std::string timestamp = to_string_timestamp(datetime);
	return write_line(timestamp + " " + entry);
}

Optional<File::Error> LogFile::write_line(const std::string& message) {
	auto error = file.write_line(message);
	if( !error.is_valid() ) {
		file.sync();
	}
	return error;
}

void DEBUG(const uint32_t Type, std::string logline) {

	rtc::RTC datetime;
	File file;
	rtcGetTime(&RTCD1, &datetime);
	
	auto text1 =to_string_dec_uint(chCoreStatus())+":" + 		
				to_string_dec_uint(datetime.hour(), 2, '0') + ":" +
				to_string_dec_uint(datetime.minute(), 2, '0') + ":" +
				to_string_dec_uint(datetime.second(), 2, '0') + ":" ;  
	
	switch (Type) 	{
					case 0:		logline = "-INF-" + logline	;  	break ;
					case 1: 	logline = "-ERR-" + logline	;	break ;
					case 2:    	logline = "-DEB-" + logline ; 	break ;	
					case 3:				break ;    // no info
					default:	logline = "-ERROR-TYPE !!!!!" + logline	; 
					}			
logline = text1 + logline ;  
		 
	if (sdcIsCardInserted(&SDCD1)) {		  	
		
		file.append(u"DEBUG.TXT");
		file.write_line_n(logline); 
		

//		text_AD.set(logline);	
	//chThdSleepMilliseconds(5000);
	
	}
	else {						// NO SD CARD
							// NEED TO PRINT ON SCREEN PLUS TIMER
/*
text_AD.set(logline);	
chThdSleepMilliseconds(5000);
*/
	}
	
	}

