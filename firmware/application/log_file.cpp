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
	open();

	sd_card_status_signal_token = sd_card::status_signal += [this](const sd_card::Status status) {
		this->on_sd_card_status(status);
	};
}

LogFile::~LogFile() {
	sd_card::status_signal -= sd_card_status_signal_token;

	close();
}

bool LogFile::open() {
	const auto open_result = f_open(&f, file_path.c_str(), FA_WRITE | FA_OPEN_ALWAYS);
	if( open_result == FR_OK ) {
		const auto seek_result = f_lseek(&f, f_size(&f));
		if( seek_result == FR_OK ) {
			return true;
		} else {
			close();
		}
	}

	return false;
}

bool LogFile::close() {
	f_close(&f);
	return true;
}

bool LogFile::is_ready() {
	return !f_error(&f);
}

bool LogFile::write_entry(const rtc::RTC& datetime, const std::string& entry) {
	std::string timestamp = to_string_timestamp(datetime);
	return write(timestamp + " " + entry + "\r\n");
}

bool LogFile::write(const std::string& message) {
	const auto puts_result = f_puts(message.c_str(), &f);
	const auto sync_result = f_sync(&f);
	return (puts_result >= 0) && (sync_result == FR_OK);
}

void LogFile::on_sd_card_status(const sd_card::Status status) {
	if( status == sd_card::Status::Mounted ) {
		open();
	} else {
		close();		
	}
}
