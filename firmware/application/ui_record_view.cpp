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

namespace ui {

RecordView::RecordView(
	const Rect parent_rect,
	std::string filename_stem_pattern,
	std::string filename_extension,
	const size_t buffer_size_k,
	const size_t buffer_count_k
) : View { parent_rect },
	filename_stem_pattern { filename_stem_pattern },
	filename_extension { filename_extension },
	buffer_size_k { buffer_size_k },
	buffer_count_k { buffer_count_k }
{
	add_children({ {
		&button_record,
		&text_record_filename,
		&text_record_dropped,
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

	if( sampling_rate == 0 ) {
		return;
	}

	const auto filename_stem = next_filename_stem_matching_pattern(filename_stem_pattern);
	text_record_filename.set(filename_stem);
	text_record_dropped.set("");
	if( filename_stem.empty() ) {
		return;
	}

	write_metadata_file(filename_stem + ".TXT");

	capture_thread = std::make_unique<CaptureThread>(filename_stem + filename_extension, buffer_size_k, buffer_count_k);
	button_record.set_bitmap(&bitmap_stop);
}

void RecordView::stop() {
	if( is_active() ) {
		capture_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	}
}

void RecordView::write_metadata_file(const std::string& filename) {
	File file;
	file.open_for_writing(filename);
	file.puts("sample_rate=" + to_string_dec_uint(sampling_rate) + "\n");
	file.puts("center_frequency=" + to_string_dec_uint(receiver_model.tuning_frequency()) + "\n");
}

void RecordView::on_tick_second() {
	if( is_active() ) {
		const auto dropped_percent = std::min(99U, capture_thread->state().dropped_percent());
		const auto s = to_string_dec_uint(dropped_percent, 2, ' ') + "\%";
		text_record_dropped.set(s);
	}
}

} /* namespace ui */
