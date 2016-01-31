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

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

LogFile::LogFile(
	const std::string file_path
) : file_path { file_path }
{
	file.open_for_append(file_path);

	sd_card_status_signal_token = sd_card::status_signal += [this](const sd_card::Status status) {
		this->on_sd_card_status(status);
	};
}

LogFile::~LogFile() {
	sd_card::status_signal -= sd_card_status_signal_token;

	file.close();
}

bool LogFile::is_ready() {
	return file.is_ready();
}

bool LogFile::write_entry(const rtc::RTC& datetime, const std::string& entry) {
	std::string timestamp = to_string_timestamp(datetime);
	return write(timestamp + " " + entry + "\r\n");
}

bool LogFile::write(const std::string& message) {
	return file.puts(message) && file.sync();
}

void LogFile::on_sd_card_status(const sd_card::Status status) {
	if( status == sd_card::Status::Mounted ) {
		file.open_for_append(file_path);
	} else {
		file.close();		
	}
}
