/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "io_wave.hpp"

Optional<File::Error> WAVFileWriter::create(
	const std::filesystem::path& filename,
	size_t sampling_rate
) {
	sampling_rate = sampling_rate;
	const auto create_error = FileWriter::create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	} else {
		return update_header();
	}
}

Optional<File::Error> WAVFileWriter::update_header() {
	header_t header { sampling_rate, bytes_written };
	const auto seek_0_result = file.seek(0);
	if( seek_0_result.is_error() ) {
		return seek_0_result.error();
	}
	const auto old_position = seek_0_result.value();
	const auto write_result = file.write(&header, sizeof(header));
	if( write_result.is_error() ) {
		return write_result.error();
	}
	const auto seek_old_result = file.seek(old_position);
	if( seek_old_result.is_error() ) {
		return seek_old_result.error();
	}
	return { };
}
