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

#ifndef __UI_RECORD_VIEW_H__
#define __UI_RECORD_VIEW_H__

#include "ui_widget.hpp"

#include "capture_thread.hpp"
#include "signal.hpp"

#include "bitmap.hpp"

#include <cstddef>
#include <string>
#include <memory>

namespace ui {

class RecordView : public View {
public:
	std::function<void(std::string)> on_error { };

	enum FileType {
		RawS16 = 2,
		WAV = 3,
	};

	RecordView(
		const Rect parent_rect,
		std::filesystem::path filename_stem_pattern,
		FileType file_type,
		const size_t write_size,
		const size_t buffer_count
	);
	~RecordView();

	void focus() override;

	void set_sampling_rate(const size_t new_sampling_rate);

	void start();
	void stop();
	void on_hide() override;

	bool is_active() const;

private:
	void toggle();
	//void toggle_pitch_rssi();
	Optional<File::Error> write_metadata_file(const std::filesystem::path& filename);

	void on_tick_second();
	void update_status_display();

	void handle_capture_thread_done(const File::Error error);
	void handle_error(const File::Error error);

	//bool pitch_rssi_enabled = false;
	const std::filesystem::path filename_stem_pattern;
	const FileType file_type;
	const size_t write_size;
	const size_t buffer_count;
	size_t sampling_rate { 0 };
	SignalToken signal_token_tick_second { };

	Rectangle rect_background {
		Color::black()
	};
	
	/*ImageButton button_pitch_rssi {
		{ 2, 0 * 16, 3 * 8, 1 * 16 },
		&bitmap_rssipwm,
		Color::orange(),
		Color::black()
	};*/

	ImageButton button_record {
		//{ 4 * 8, 0 * 16, 2 * 8, 1 * 16 },
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		&bitmap_record,
		Color::red(),
		Color::black()
	};

	Text text_record_filename {
		{ 7 * 8, 0 * 16, 8 * 8, 16 },
		"",
	};

	Text text_record_dropped {
		{ 16 * 8, 0 * 16, 3 * 8, 16 },
		"",
	};

	Text text_time_available {
		{ 21 * 8, 0 * 16, 9 * 8, 16 },
		"",
	};

	std::unique_ptr<CaptureThread> capture_thread { };

	MessageHandlerRegistration message_handler_capture_thread_error {
		Message::ID::CaptureThreadDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const CaptureThreadDoneMessage*>(p);
			this->handle_capture_thread_done(message.error);
		}
	};
};

} /* namespace ui */

#endif/*__UI_RECORD_VIEW_H__*/
