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

#include "ui_record_view.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "file.hpp"
#include "time.hpp"

#include "string_format.hpp"
#include "utility.hpp"

#include <cstdint>

class RawFileWriter : public Writer {
public:
	RawFileWriter(
		const std::string& filename
	) : file { filename, File::openmode::out | File::openmode::binary | File::openmode::trunc }
	{
	}

	bool write(const void* const buffer, const size_t bytes) override {
		return file.write(buffer, bytes);
	}

private:
	File file;
};

class WAVFileWriter : public Writer {
public:
	WAVFileWriter(
		const std::string& filename,
		size_t sampling_rate
	) : file { filename, File::openmode::out | File::openmode::binary | File::openmode::trunc },
		header { sampling_rate }
	{
		update_header();
	}

	~WAVFileWriter() {
		update_header();
	}

	bool write(const void* const buffer, const size_t bytes) override {
		const auto success = file.write(buffer, bytes) ;
		if( success ) {
			bytes_written += bytes;
		}
		return success;
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

	File file;
	header_t header;
	uint64_t bytes_written { 0 };

	void update_header() {
		header.set_data_size(bytes_written);
		const auto old_position = file.seek(0);
		file.write(&header, sizeof(header));
		file.seek(old_position);
	}
};

namespace ui {

RecordView::RecordView(
	const Rect parent_rect,
	std::string filename_stem_pattern,
	const FileType file_type,
	const size_t write_size,
	const size_t buffer_count
) : View { parent_rect },
	filename_stem_pattern { filename_stem_pattern },
	file_type { file_type },
	write_size { write_size },
	buffer_count { buffer_count }
{
	add_children({ {
		&button_record,
		&text_record_filename,
		&text_record_dropped,
		&text_time_available,
	} });

	button_record.on_select = [this](ImageButton&) {
		this->toggle();
	};

	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
}

RecordView::~RecordView() {
	time::signal_tick_second -= signal_token_tick_second;
}

void RecordView::focus() {
	button_record.focus();
}

bool RecordView::is_active() const {
	return (bool)capture_thread;
}

void RecordView::toggle() {
	if( is_active() ) {
		stop();
	} else {
		start();
	}
}

void RecordView::start() {
	stop();

	text_record_filename.set("");
	text_record_dropped.set("");

	if( sampling_rate == 0 ) {
		return;
	}

	const auto filename_stem = next_filename_stem_matching_pattern(filename_stem_pattern);
	if( filename_stem.empty() ) {
		return;
	}

	std::unique_ptr<Writer> writer;
	switch(file_type) {
	case FileType::WAV:
		writer = std::make_unique<WAVFileWriter>(
			filename_stem + ".WAV",
			sampling_rate
		);
		break;

	case FileType::RawS16:
		write_metadata_file(filename_stem + ".TXT");
		writer = std::make_unique<RawFileWriter>(
			filename_stem + ".C16"
		);
		break;

	default:
		break;
	};

	if( writer ) {
		text_record_filename.set(filename_stem);
		button_record.set_bitmap(&bitmap_stop);
		capture_thread = std::make_unique<CaptureThread>(
			std::move(writer),
			write_size, buffer_count
		);
	}
}

void RecordView::stop() {
	if( is_active() ) {
		capture_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	}
}

void RecordView::write_metadata_file(const std::string& filename) {
	File file { filename, File::openmode::out | File::openmode::trunc };
	file.puts("sample_rate=" + to_string_dec_uint(sampling_rate) + "\n");
	file.puts("center_frequency=" + to_string_dec_uint(receiver_model.tuning_frequency()) + "\n");
}

void RecordView::on_tick_second() {
	if( is_active() ) {
		const auto dropped_percent = std::min(99U, capture_thread->state().dropped_percent());
		const auto s = to_string_dec_uint(dropped_percent, 2, ' ') + "\%";
		text_record_dropped.set(s);

		const auto space_info = std::filesystem::space("");
		const uint32_t bytes_per_second = file_type == FileType::WAV ? (sampling_rate * 2) : (sampling_rate * 4);
		const uint32_t available_seconds = space_info.free / bytes_per_second;
		text_time_available.set(to_string_dec_uint(available_seconds, 6, ' ') + "s");
	}
}

} /* namespace ui */
