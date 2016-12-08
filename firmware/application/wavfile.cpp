/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "wavfile.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "file.hpp"
#include "time.hpp"
#include "utility.hpp"

#include <cstdint>

namespace ui {

int WAVFileReader::open(const std::string& filename) {
	if (filename == last_filename) {
		rewind();
		return 1;			// Already open
	}
	
	auto error = file.open(filename);

	if (!error.is_valid()) {
		file.read((void*)&header, sizeof(header));
		
		// TODO: Check validity here
	
		last_filename = filename;
		data_start = header.fmt.cksize + 28;		// Skip "data" and cksize
		
		data_size_ = header.data.cksize;
		sample_rate_ = header.fmt.nSamplesPerSec;
		bytes_per_sample = header.fmt.wBitsPerSample / 8;
		
		rewind();
		
		return 1;
	} else {
		return 0;
	}
}

size_t WAVFileReader::read(void * const data, const size_t bytes_to_read) {
	return file.read(data, bytes_to_read).value();
}

void WAVFileReader::rewind() {
	file.seek(data_start);
}

uint32_t WAVFileReader::ms_duration() {
	return ((data_size_ * 1000) / sample_rate_) / bytes_per_sample;
}

int WAVFileReader::seek_mss(const uint16_t minutes, const uint8_t seconds, const uint32_t samples) {
	const auto result = file.seek(data_start + ((((minutes * 60) + seconds) * sample_rate_) + samples) * bytes_per_sample);

	if (result.is_error())
		return 0;
		
	return 1;
}

uint16_t WAVFileReader::channels() {
	return header.fmt.nChannels;
}

uint32_t WAVFileReader::sample_rate() {
	return sample_rate_;
}

uint32_t WAVFileReader::data_size() {
	return data_size_;
}

uint16_t WAVFileReader::bits_per_sample() {
	return header.fmt.wBitsPerSample;
}

WAVFileWriter::~WAVFileWriter() {
	update_header();
}

Optional<File::Error> WAVFileWriter::create(
	const std::string& filename
) {
	const auto create_error = FileWriter::create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	} else {
		return update_header();
	}
}

Optional<File::Error> WAVFileWriter::update_header() {
	header.set_data_size(bytes_written);
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

} /* namespace ui */
