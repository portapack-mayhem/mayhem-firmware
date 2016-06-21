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

class FileWriter : public Writer {
public:
	FileWriter() = default;

	FileWriter(const FileWriter&) = delete;
	FileWriter& operator=(const FileWriter&) = delete;
	FileWriter(FileWriter&& file) = delete;
	FileWriter& operator=(FileWriter&&) = delete;

	Optional<File::Error> create(const std::string& filename) {
		return file.create(filename);
	}

	File::Result<size_t> write(const void* const buffer, const size_t bytes) override {
		auto write_result = file.write(buffer, bytes) ;
		if( write_result.is_ok() ) {
			bytes_written += write_result.value();
		}
		return write_result;
	}

protected:
	File file;
	uint64_t bytes_written { 0 };
};

using RawFileWriter = FileWriter;

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
		const std::string& filename
	) {
		const auto create_error = FileWriter::create(filename);
		if( create_error.is_valid() ) {
			return create_error;
		} else {
			update_header();
			return { };
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

	void update_header() {
		header.set_data_size(bytes_written);
		auto seek_result = file.seek(0);
		if( seek_result.is_error() ) {
			return;
		}
		const auto old_position = seek_result.value();
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
		&rect_background,
		&button_record,
		&text_record_filename,
		&text_record_dropped,
		&text_time_available,
	} });

	rect_background.set_parent_rect({ { 0, 0 }, size() });

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
		{
			auto p = std::make_unique<WAVFileWriter>(
				sampling_rate
			);
			auto create_error = p->create(
				filename_stem + ".WAV"
			);
			if( create_error.is_valid() ) {
				report_error(create_error.value().what());
			} else {
				writer = std::move(p);
			}
		}
		break;

	case FileType::RawS16:
		{
			const auto metadata_file_error = write_metadata_file(filename_stem + ".TXT");
			if( metadata_file_error.is_valid() ) {
				report_error(metadata_file_error.value().what());
				return;
			}

			auto p = std::make_unique<RawFileWriter>();
			auto create_error = p->create(
				filename_stem + ".C16"
			);
			if( create_error.is_valid() ) {
				report_error(create_error.value().what());
			} else {
				writer = std::move(p);
			}
		}
		break;

	default:
		break;
	};

	if( writer ) {
		text_record_filename.set(filename_stem);
		button_record.set_bitmap(&bitmap_stop);
		capture_thread = std::make_unique<CaptureThread>(
			std::move(writer),
			write_size, buffer_count,
			[](File::Error error) {
				CaptureThreadErrorMessage message { error.code() };
				EventDispatcher::send_message(message);
			}
		);
	}
}

void RecordView::stop() {
	if( is_active() ) {
		capture_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	}
}

Optional<File::Error> RecordView::write_metadata_file(const std::string& filename) {
	File file;
	const auto create_error = file.create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	} else {
		const auto puts_result1 = file.puts("sample_rate=" + to_string_dec_uint(sampling_rate) + "\n");
		if( puts_result1.is_error() ) {
			return { puts_result1.error() };
		}
		const auto puts_result2 = file.puts("center_frequency=" + to_string_dec_uint(receiver_model.tuning_frequency()) + "\n");
		if( puts_result2.is_error() ) {
			return { puts_result2.error() };
		}
		return { };
	}
}

void RecordView::on_tick_second() {
	if( is_active() ) {
		const auto dropped_percent = std::min(99U, capture_thread->state().dropped_percent());
		const auto s = to_string_dec_uint(dropped_percent, 2, ' ') + "\%";
		text_record_dropped.set(s);
	}

	if( sampling_rate ) {
		const auto space_info = std::filesystem::space("");
		const uint32_t bytes_per_second = file_type == FileType::WAV ? (sampling_rate * 2) : (sampling_rate * 4);
		const uint32_t available_seconds = space_info.free / bytes_per_second;
		const uint32_t seconds = available_seconds % 60;
		const uint32_t available_minutes = available_seconds / 60;
		const uint32_t minutes = available_minutes % 60;
		const uint32_t hours = available_minutes / 60;
		const std::string available_time =
			to_string_dec_uint(hours, 3, ' ') + ":" +
			to_string_dec_uint(minutes, 2, '0') + ":" +
			to_string_dec_uint(seconds, 2, '0');
		text_time_available.set(available_time);
	}
}

void RecordView::report_error(const std::string& message) {
	stop();
	if( on_error ) {
		on_error(message);
	}
}

} /* namespace ui */
