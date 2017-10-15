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

#include "io_wave.hpp"

bool WAVFileReader::open(const std::filesystem::path& path) {
	size_t i = 0;
	char ch;
	const uint8_t tag_INAM[4] = { 'I', 'N', 'A', 'M' };
	char title_buffer[32];
	uint32_t riff_size, data_end, title_size;
	size_t search_limit = 0;
	
	// Already open ?
	if (path.string() == last_path.string()) {
		rewind();
		return true;
	}
	
	auto error = file.open(path);

	if (!error.is_valid()) {
		file.read((void*)&header, sizeof(header));	// Read header (RIFF and WAVE)
		
		riff_size = header.cksize + 8;
		data_start = header.fmt.cksize + 28;
		data_size_ = header.data.cksize;
		data_end = data_start + data_size_ + 1;
		
		// Look for INAM (title) tag
		if (data_end < riff_size) {
			file.seek(data_end);
			while(file.read((void*)&ch, 1).is_ok()) {
				if (ch == tag_INAM[i++]) {
					if (i == 4) {
						// Tag found, copy title
						file.read((void*)&title_size, sizeof(uint32_t));
						if (title_size > 32) title_size = 32;
						file.read((void*)&title_buffer, title_size);
						title_string = title_buffer;
						break;
					}
				} else {
					if (ch == tag_INAM[0])
						i = 1;
					else
						i = 0;
				}
				if (search_limit == 256)
					break;
				else
					search_limit++;
			}
		}
		
		sample_rate_ = header.fmt.nSamplesPerSec;
		bytes_per_sample = header.fmt.wBitsPerSample / 8;
		
		rewind();
		
		last_path = path;
		
		return true;
	} else {
		return false;
	}
}

void WAVFileReader::rewind() {
	file.seek(data_start);
}

std::string WAVFileReader::title() {
	return title_string;
}

uint32_t WAVFileReader::ms_duration() {
	return ((data_size_ * 1000) / sample_rate_) / bytes_per_sample;
}

void WAVFileReader::data_seek(const uint64_t Offset) {
	file.seek(data_start + (Offset * bytes_per_sample));
}
	
/*int WAVFileReader::seek_mss(const uint16_t minutes, const uint8_t seconds, const uint32_t samples) {
	const auto result = file.seek(data_start + ((((minutes * 60) + seconds) * sample_rate_) + samples) * bytes_per_sample);

	if (result.is_error())
		return 0;
		
	return 1;
}*/

uint16_t WAVFileReader::channels() {
	return header.fmt.nChannels;
}

uint32_t WAVFileReader::sample_rate() {
	return sample_rate_;
}

uint32_t WAVFileReader::data_size() {
	return data_size_;
}

uint32_t WAVFileReader::sample_count() {
	return data_size_ / bytes_per_sample;
}

uint16_t WAVFileReader::bits_per_sample() {
	return header.fmt.wBitsPerSample;
}

Optional<File::Error> WAVFileWriter::create(
	const std::filesystem::path& filename,
	size_t sampling_rate_set,
	const std::string& title_set
) {
	sampling_rate = sampling_rate_set;
	title = title_set;
	const auto create_error = FileWriter::create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	} else {
		return update_header();
	}
}

Optional<File::Error> WAVFileWriter::update_header() {
	header_t header { sampling_rate, (uint32_t)bytes_written - sizeof(header_t), info_chunk_size };
	
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

Optional<File::Error> WAVFileWriter::write_tags() {
	tags_t tags { title };
	
	const auto write_result = file.write(&tags, sizeof(tags));
	if( write_result.is_error() ) {
		return write_result.error();
	}
	
	info_chunk_size = sizeof(tags);
	
	return { };
}
