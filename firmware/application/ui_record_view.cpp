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
#include "message.hpp"
#include "portapack_shared_memory.hpp"
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
		&button_pwmrssi,
		&button_record,
		&text_record_filename,
		&text_record_dropped,
		&text_time_available,
	} });

	rect_background.set_parent_rect({ { 0, 0 }, size() });
	
	button_pwmrssi.on_select = [this](ImageButton&) {
		this->toggle_pwmrssi();
	};

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

void RecordView::set_sampling_rate(const size_t new_sampling_rate) {
	if( new_sampling_rate != sampling_rate ) {
		stop();
		sampling_rate = new_sampling_rate;

		button_record.hidden(sampling_rate == 0);
		text_record_filename.hidden(sampling_rate == 0);
		text_record_dropped.hidden(sampling_rate == 0);
		text_time_available.hidden(sampling_rate == 0);
		rect_background.hidden(sampling_rate != 0);

		update_status_display();
	}
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

void RecordView::toggle_pwmrssi() {
	pwmrssi_enabled = !pwmrssi_enabled;
	
	// Send to RSSI widget
	const PWMRSSIConfigureMessage message {
		pwmrssi_enabled,
		1000,
		0
	};
	shared_memory.application_queue.push(message);
	
	if( !pwmrssi_enabled ) {
		button_pwmrssi.set_foreground(Color::orange());
	} else {
		button_pwmrssi.set_foreground(Color::green());
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
				handle_error(create_error.value());
			} else {
				writer = std::move(p);
			}
		}
		break;

	case FileType::RawS16:
		{
			const auto metadata_file_error = write_metadata_file(filename_stem + ".TXT");
			if( metadata_file_error.is_valid() ) {
				handle_error(metadata_file_error.value());
				return;
			}

			auto p = std::make_unique<RawFileWriter>();
			auto create_error = p->create(
				filename_stem + ".C16"
			);
			if( create_error.is_valid() ) {
				handle_error(create_error.value());
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
			[]() {
				CaptureThreadDoneMessage message { };
				EventDispatcher::send_message(message);
			},
			[](File::Error error) {
				CaptureThreadDoneMessage message { error.code() };
				EventDispatcher::send_message(message);
			}
		);
	}

	update_status_display();
}

void RecordView::stop() {
	if( is_active() ) {
		capture_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	}

	update_status_display();
}

Optional<File::Error> RecordView::write_metadata_file(const std::string& filename) {
	File file;
	const auto create_error = file.create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	} else {
		const auto error_line1 = file.write_line("sample_rate=" + to_string_dec_uint(sampling_rate));
		if( error_line1.is_valid() ) {
			return error_line1;
		}
		const auto error_line2 = file.write_line("center_frequency=" + to_string_dec_uint(receiver_model.tuning_frequency()));
		if( error_line2.is_valid() ) {
			return error_line2;
		}
		return { };
	}
}

void RecordView::on_tick_second() {
	update_status_display();
}

void RecordView::update_status_display() {
	if( is_active() ) {
		const auto dropped_percent = std::min(99U, capture_thread->state().dropped_percent());
		const auto s = to_string_dec_uint(dropped_percent, 2, ' ') + "\%";
		text_record_dropped.set(s);
	}
	
	if (pwmrssi_enabled) {
		button_pwmrssi.invert_colors();
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

void RecordView::handle_capture_thread_done(const File::Error error) {
	stop();
	if( error.code() ) {
		handle_error(error);
	}
}

void RecordView::handle_error(const File::Error error) {
	if( on_error ) {
		on_error(error.what());
	}
}

} /* namespace ui */
