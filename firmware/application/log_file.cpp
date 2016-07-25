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
