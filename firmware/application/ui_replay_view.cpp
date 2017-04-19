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

#include "rtc_time.hpp"
#include "io_file.hpp"

#include "string_format.hpp"
#include "utility.hpp"

#include <cstdint>

namespace ui {

void ReplayView::on_file_changed(const uint32_t duration) {
	std::string str_duration = "";
	
	if (duration >= 60)
		str_duration = to_string_dec_uint(duration / 60) + "m";
	
	text_duration.set(str_duration + to_string_dec_uint(duration % 60) + "s");
}

ReplayView::ReplayView(
	const Rect parent_rect,
	const size_t read_size,
	const size_t buffer_count
) : View { parent_rect },
	read_size { read_size },
	buffer_count { buffer_count }
{
	add_children({
		&rect_background,
		&button_play,
		&options_files,
		&text_duration,
		//&text_time_seek,
	});

	rect_background.set_parent_rect({ { 0, 0 }, size() });

	options_files.on_change = [this](size_t, int32_t duration) {
		this->on_file_changed(duration);
	};
	
	button_play.on_select = [this](ImageButton&) {
		this->toggle();
	};

	/*signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};*/
}

ReplayView::~ReplayView() {
	//rtc_time::signal_tick_second -= signal_token_tick_second;
}

void ReplayView::focus() {
	options_files.focus();
}

void ReplayView::set_file_list(const std::vector<std::filesystem::path>& file_list) {
	File bbd_file;
	uint32_t duration;
	
	for (const auto& file : file_list) {
		bbd_file.open("/" + file.string());
		duration = bbd_file.size() / (2 * 2 * (sampling_rate / 8));
		file_options.emplace_back(file.string().substr(0, 8), duration);
	}
	options_files.set_options(file_options);
	options_files.set_selected_index(0);	// First file
	on_file_changed(file_options[0].second);
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

	auto reader = std::make_unique<FileReader>();

	if( reader ) {
		button_play.set_bitmap(&bitmap_stop);
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
	
	radio::enable({
		434000000,	//target_frequency(),
		sampling_rate,
		2500000,	//baseband_bandwidth,
		rf::Direction::Transmit,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga())
	});
}

void ReplayView::stop() {
	if( is_active() ) {
		replay_thread.reset();
		radio::disable();
		button_play.set_bitmap(&bitmap_play);
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
