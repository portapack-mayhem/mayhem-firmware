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

#pragma once

#include "io_file.hpp"

#include "file.hpp"
#include "optional.hpp"

#include <cstddef>
#include <cstdint>

class WAVFileWriter : public FileWriter {
public:
	WAVFileWriter(
		size_t sampling_rate
	) : header { sampling_rate }
	{
	}


	WAVFileWriter(const WAVFileWriter&) = delete;
	WAVFileWriter& operator=(const WAVFileWriter&) = delete;
	WAVFileWriter(WAVFileWriter&&) = delete;
	WAVFileWriter& operator=(WAVFileWriter&&) = delete;

	~WAVFileWriter() {
		update_header();
	}

	Optional<File::Error> create(
		const std::filesystem::path& filename
	) {
		const auto create_error = FileWriter::create(filename);
		if( create_error.is_valid() ) {
			return create_error;
		} else {
			return update_header();
		}
	}

private:
	struct fmt_pcm_t {
		constexpr fmt_pcm_t(
			const uint32_t sampling_rate
		) : nSamplesPerSec { sampling_rate },
			nAvgBytesPerSec { nSamplesPerSec * nBlockAlign }
		{
		}

	private:
		const uint8_t ckID[4] { 'f', 'm', 't', ' ' };
		const uint32_t cksize { 16 };
		const uint16_t wFormatTag { 0x0001 };
		const uint16_t nChannels { 1 };
		const uint32_t nSamplesPerSec;
		const uint32_t nAvgBytesPerSec;
		const uint16_t nBlockAlign { 2 };
		const uint16_t wBitsPerSample { 16 };
	};

	struct data_t {
		void set_size(const uint32_t value) {
			cksize = value;
		}

	private:
		const uint8_t ckID[4] { 'd', 'a', 't', 'a' };
		uint32_t cksize { 0 };
	};

	struct header_t {
		constexpr header_t(
			const uint32_t sampling_rate
		) : fmt { sampling_rate }
		{
		}

		void set_data_size(const uint32_t value) {
			data.set_size(value);
			cksize = sizeof(header_t) + value - 8;
		}

	private:
		const uint8_t riff_id[4] { 'R', 'I', 'F', 'F' };
		uint32_t cksize { 0 };
		const uint8_t wave_id[4] { 'W', 'A', 'V', 'E' };
		fmt_pcm_t fmt;
		data_t data;
	};

	header_t header;

	Optional<File::Error> update_header() {
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
};
