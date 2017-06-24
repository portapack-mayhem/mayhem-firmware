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

#ifndef __UI_REPLAY_VIEW_H__
#define __UI_REPLAY_VIEW_H__

#include "ui_widget.hpp"

#include "replay_thread.hpp"
#include "signal.hpp"
#include "bitmap.hpp"

#include <cstddef>
#include <string>
#include <memory>

namespace ui {

class ReplayView : public View {
public:
	std::function<void(std::string)> on_error { };

	ReplayView(
		const Rect parent_rect,
		const size_t read_size,
		const size_t buffer_count
	);
	~ReplayView();

	void focus() override;

	void set_file_list(const std::vector<std::filesystem::path>& file_list);

	void start();
	void stop();

	bool is_active() const;

private:
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	
	static constexpr uint32_t sampling_rate = 4000000;
	
	void toggle();

	void on_file_changed(const uint32_t duration);
	void on_tick_second();
	void update_status_display();

	void handle_replay_thread_done(const File::Error error);
	void handle_error(const File::Error error);

	const size_t read_size;
	const size_t buffer_count;
	//SignalToken signal_token_tick_second { };
	options_t file_options { };
	//std::filesystem:path file_path { };

	Rectangle rect_background {
		Color::black()
	};

	ImageButton button_play {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		&bitmap_play,
		Color::green(),
		Color::black()
	};

	OptionsField options_files {
		{ 2 * 8, 0 * 8 },
		8,
		{ }
	};
	
	Text text_duration {
		{ 11 * 8, 0 * 8, 12 * 8, 16 },
		"-"
	};

	/*Text text_time_seek {
		{ 18 * 8, 0 * 16, 9 * 8, 16 },
		"",
	};*/

	std::unique_ptr<ReplayThread> replay_thread { };

	MessageHandlerRegistration message_handler_capture_thread_error {
		Message::ID::CaptureThreadDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
			this->handle_replay_thread_done(message.error);
		}
	};
};

} /* namespace ui */

#endif/*__UI_REPLAY_VIEW_H__*/
