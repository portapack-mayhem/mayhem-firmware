/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_SOUNDBOARD_H__
#define __UI_SOUNDBOARD_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "replay_thread.hpp"
#include "baseband_api.hpp"
#include "ui_receiver.hpp"
#include "io_wave.hpp"
#include "tone_key.hpp"

namespace ui {

class SoundBoardView : public View {
public:
	SoundBoardView(NavigationView& nav);
	~SoundBoardView();

	SoundBoardView(const SoundBoardView&) = delete;
	SoundBoardView(SoundBoardView&&) = delete;
	SoundBoardView& operator=(const SoundBoardView&) = delete;
	SoundBoardView& operator=(SoundBoardView&&) = delete;

	void focus() override;
	
	std::string title() const override { return "Soundboard"; };
	
private:
	NavigationView& nav_;
	
	enum tx_modes {
		NORMAL = 0,
		RANDOM
	};
	
	tx_modes tx_mode = NORMAL;
	
	struct sound {
		std::filesystem::path path { };
		uint32_t ms_duration = 0;
		std::string title { };
	};
	
	uint32_t sample_counter { 0 };
	uint32_t sample_duration { 0 };
	uint8_t page = 0;
	
	uint32_t lfsr_v = 0x13377331;
	
	sound sounds[60];			// 5 pages * 12 buttons
	uint32_t max_sound { };
	uint8_t max_page { };
	uint32_t playing_id { };

	const size_t read_size { 2048 };	// Less ?
	const size_t buffer_count { 3 };
	std::unique_ptr<ReplayThread> replay_thread { };
	bool ready_signal { false };
	
	Style style_a {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 255, 51, 153 }
	};
	Style style_b {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 153, 204, 0 }
	};
	Style style_c {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 51, 204, 204 }
	};
	Style style_d {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 153, 102, 255 }
	};

	std::array<Button, 15> buttons { };
	const Style * styles[4] = { &style_a, &style_b, &style_c, &style_d };
	
	void on_tuning_frequency_changed(rf::Frequency f);
	
	void do_random();
	void show_infos(uint16_t id);
	bool change_page(Button& button, const KeyEvent key);
	void refresh_buttons(uint16_t id);
	void play_sound(uint16_t id);
	void on_ctcss_changed(uint32_t v);
	void stop(const bool do_loop);
	bool is_active() const;
	void set_ready();
	void handle_replay_thread_done(const uint32_t return_code);
	void file_error();
	void on_tx_progress(const uint32_t progress);
	
	Labels labels {
		{ { 10 * 8, 4 }, "BW:   kHz", Color::light_grey() }
	};
	
	FrequencyField field_frequency {
		{ 0, 4 },
	};
	
	NumberField number_bw {
		{ 13 * 8, 4 },
		3,
		{ 1, 150 },
		1,
		' '
	};
	
	OptionsField options_tone_key {
		{ 21 * 8, 4 },
		8,
		{ }
	};
	
	Text text_title {
		{ 1 * 8, 28 * 8, 20 * 8, 16 },
		"-"
	};
	
	Text text_page {
		{ 22 * 8 - 4, 28 * 8, 8 * 8, 16 },
		"Page -/-"
	};
	
	Text text_duration {
		{ 1 * 8, 31 * 8, 5 * 8, 16 }
	};
	
	ProgressBar progressbar {
		{ 9 * 8, 31 * 8, 20 * 8, 16 }
	};
	
	Checkbox check_loop {
		{ 8, 274 },
		4,
		"Loop"
	};
	
	Button button_random {
		{ 10 * 8, 34 * 8, 9 * 8, 32 },
		"Random"
	};
	
	Button button_exit {
		{ 21 * 8, 34 * 8, 8 * 8, 32 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_replay_thread_error {
		Message::ID::ReplayThreadDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
			this->handle_replay_thread_done(message.return_code);
		}
	};
	
	MessageHandlerRegistration message_handler_fifo_signal {
		Message::ID::RequestSignal,
		[this](const Message* const p) {
			const auto message = static_cast<const RequestSignalMessage*>(p);
			if (message->signal == RequestSignalMessage::Signal::FillRequest) {
				this->set_ready();
			}
		}
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.progress);
		}
	};
};

} /* namespace ui */

#endif/*__UI_SOUNDBOARD_H__*/
