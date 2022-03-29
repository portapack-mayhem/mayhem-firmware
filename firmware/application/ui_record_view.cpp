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

#include "io_file.hpp"
#include "io_wave.hpp"

#include "baseband_api.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "utility.hpp"

#include <cstdint>

namespace ui {

/*void RecordView::toggle_pitch_rssi() {
	pitch_rssi_enabled = !pitch_rssi_enabled;
	
	// Send to RSSI widget
	const PitchRSSIConfigureMessage message {
		pitch_rssi_enabled,
		0
	};
	shared_memory.application_queue.push(message);
	
	if( !pitch_rssi_enabled ) {
		button_pitch_rssi.set_foreground(Color::orange());
	} else {
		button_pitch_rssi.set_foreground(Color::green());
	}
}*/

RecordView::RecordView(
	const Rect parent_rect,
	std::filesystem::path filename_stem_pattern,
	const FileType file_type,
	const size_t write_size,
	const size_t buffer_count
) : View { parent_rect },
	filename_stem_pattern { filename_stem_pattern },
	file_type { file_type },
	write_size { write_size },
	buffer_count { buffer_count }
{
	add_children({
		&rect_background,
		//&button_pitch_rssi,
		&button_record,
		&text_record_filename,
		&text_record_dropped,
		&text_time_available,
	});
	
	rect_background.set_parent_rect({ { 0, 0 }, size() });
	
	/*button_pitch_rssi.on_select = [this](ImageButton&) {
		this->toggle_pitch_rssi();
	};*/

	button_record.on_select = [this](ImageButton&) {
		this->toggle();
	};

	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
}

RecordView::~RecordView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
}

void RecordView::focus() {
	button_record.focus();
}

void RecordView::set_sampling_rate(const size_t new_sampling_rate) {
    
	/* We are changing "REC" icon background to yellow in  BW rec Options >600kHz 
	where we are NOT recording full IQ .C16 files (recorded files are decimated ones).
	Those decimated recorded files,has not the full IQ  samples . 
	are ok as recorded spectrum indication, but they  should not be used by Replay app. 
	 	
	We keep original black  background in all the correct IQ .C16 files BW's Options */ 
	if (new_sampling_rate > 4800000) {   // > BW >600kHz  (fs=8*BW), (750kHz ...2750kHz)
		button_record.set_background(ui::Color::yellow());		
	} else {
		button_record.set_background(ui::Color::black());				
	}
	
	if( new_sampling_rate != sampling_rate ) {
		stop();
		
		sampling_rate = new_sampling_rate;
		baseband::set_sample_rate(sampling_rate);

		button_record.hidden(sampling_rate == 0);
		text_record_filename.hidden(sampling_rate == 0);
		text_record_dropped.hidden(sampling_rate == 0);
		text_time_available.hidden(sampling_rate == 0);
		rect_background.hidden(sampling_rate != 0);

		update_status_display();
	}
}

// Setter for datetime and frequency filename
void RecordView::set_filename_date_frequency(bool set) {
	filename_date_frequency = set;
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

	 
    std::filesystem::path base_path;
	if(filename_date_frequency) {
     	rtcGetTime(&RTCD1, &datetime);

		//ISO 8601 
		std::string date_time = to_string_dec_uint(datetime.year(), 4, '0')  +
			                    to_string_dec_uint(datetime.month(), 2, '0') + 
			                    to_string_dec_uint(datetime.day(), 2, '0')   + "T" +
								to_string_dec_uint(datetime.hour())          +
								to_string_dec_uint(datetime.minute())        +
								to_string_dec_uint(datetime.second());

		base_path = filename_stem_pattern.string() + "_" + date_time + "_" + to_string_freq(receiver_model.tuning_frequency()) + "Hz";
	} else {
		base_path = next_filename_stem_matching_pattern(filename_stem_pattern);
	}

	if( base_path.empty() ) {
		return;
	}

	std::unique_ptr<stream::Writer> writer;
	switch(file_type) {
	case FileType::WAV:
		{
			auto p = std::make_unique<WAVFileWriter>();
			auto create_error = p->create(
				base_path.replace_extension(u".WAV"),
				sampling_rate,
				to_string_dec_uint(receiver_model.tuning_frequency()) + "Hz"
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
			const auto metadata_file_error = write_metadata_file(base_path.replace_extension(u".TXT"));
			if( metadata_file_error.is_valid() ) {
				handle_error(metadata_file_error.value());
				return;
			}

			auto p = std::make_unique<RawFileWriter>();
			auto create_error = p->create(base_path.replace_extension(u".C16"));
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
		text_record_filename.set(base_path.replace_extension().string());
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

void RecordView::on_hide() {
	stop(); // Stop current recording
	View::on_hide();
}

void RecordView::stop() {
	if( is_active() ) {
		capture_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	}

	update_status_display();
}

Optional<File::Error> RecordView::write_metadata_file(const std::filesystem::path& filename) {
	File file;
	const auto create_error = file.create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	} else {
		const auto error_line1 = file.write_line("sample_rate=" + to_string_dec_uint(sampling_rate / 8));
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
	
	/*if (pitch_rssi_enabled) {
		button_pitch_rssi.invert_colors();
	}*/

	if( sampling_rate ) {
		const auto space_info = std::filesystem::space(u"");
		const uint32_t bytes_per_second = file_type == FileType::WAV ? (sampling_rate * 2) : (sampling_rate / 8 * 4);
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
