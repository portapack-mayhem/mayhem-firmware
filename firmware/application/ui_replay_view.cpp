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

#include "ui_replay_view.hpp"

#include "portapack.hpp"
#include "message.hpp"
#include "portapack_shared_memory.hpp"
using namespace portapack;

#include "time.hpp"

#include "string_format.hpp"
#include "utility.hpp"

#include <cstdint>

namespace ui {

ReplayView::ReplayView(
	const Rect parent_rect,
	std::string filename_stem_pattern,
	const FileType file_type,
	const size_t read_size,
	const size_t buffer_count
) : View { parent_rect },
	filename_stem_pattern { filename_stem_pattern },
	file_type { file_type },
	read_size { read_size },
	buffer_count { buffer_count }
{
	add_children({ {
		&rect_background,
		&button_record,
		&text_replay_filename,
		&text_time_seek,
	} });

	rect_background.set_parent_rect({ { 0, 0 }, size() });

	button_record.on_select = [this](ImageButton&) {
		this->toggle();
	};

	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
}

ReplayView::~ReplayView() {
	time::signal_tick_second -= signal_token_tick_second;
}

void ReplayView::focus() {
	button_record.focus();
}

void ReplayView::set_sampling_rate(const size_t new_sampling_rate) {
	if( new_sampling_rate != sampling_rate ) {
		stop();
		sampling_rate = new_sampling_rate;

		button_record.hidden(sampling_rate == 0);
		text_replay_filename.hidden(sampling_rate == 0);
		text_time_seek.hidden(sampling_rate == 0);
		rect_background.hidden(sampling_rate != 0);

		update_status_display();
	}
}

bool ReplayView::is_active() const {
	return (bool)replay_thread;
}

void ReplayView::toggle() {
	if( is_active() ) {
		stop();
	} else {
		start();
	}
}

void ReplayView::start() {
	stop();

	text_replay_filename.set("");

	if( sampling_rate == 0 ) {
		return;
	}

	const auto filename_stem = next_filename_stem_matching_pattern(filename_stem_pattern);
	if( filename_stem.empty() ) {
		return;
	}

	std::unique_ptr<Reader> reader;
	auto p = std::make_unique<FileReader>();
	auto create_error = p->create(
		filename_stem + ".C16"
	);
	if( create_error.is_valid() ) {
		handle_error(create_error.value());
	} else {
		reader = std::move(p);
	}

	if( reader ) {
		text_replay_filename.set(filename_stem);
		button_record.set_bitmap(&bitmap_stop);
		replay_thread = std::make_unique<ReplayThread>(
			std::move(reader),
			read_size, buffer_count,
			[]() {
				ReplayThreadDoneMessage message { };
				EventDispatcher::send_message(message);
			},
			[](File::Error error) {
				ReplayThreadDoneMessage message { error.code() };
				EventDispatcher::send_message(message);
			}
		);
	}

	update_status_display();
}

void ReplayView::stop() {
	if( is_active() ) {
		replay_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	}

	update_status_display();
}

void ReplayView::on_tick_second() {
	update_status_display();
}

void ReplayView::update_status_display() {
	/*if( sampling_rate ) {
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
	}*/
}

void ReplayView::handle_replay_thread_done(const File::Error error) {
	stop();
	if( error.code() ) {
		handle_error(error);
	}
}

void ReplayView::handle_error(const File::Error error) {
	if( on_error ) {
		on_error(error.what());
	}
}

} /* namespace ui */
