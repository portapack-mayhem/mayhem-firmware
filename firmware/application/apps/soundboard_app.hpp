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

#include "ui_widget.hpp"
#include "ui_transmitter.hpp"
#include "replay_thread.hpp"
#include "baseband_api.hpp"
#include "lfsr_random.hpp"
#include "io_wave.hpp"
#include "tone_key.hpp"
#include "app_settings.hpp"

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
	
	std::string title() const override { return "Soundboard TX"; };
	
private:
	NavigationView& nav_;

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };
	
	enum tx_modes {
		NORMAL = 0,
		RANDOM
	};
	
	tx_modes tx_mode = NORMAL;
	
	uint32_t playing_id { };
	uint32_t page = 1;
	uint32_t c_page = 1;
	
	std::vector<std::filesystem::path> file_list { };

	const size_t read_size { 2048 };	// Less ?
	const size_t buffer_count { 3 };
	std::unique_ptr<ReplayThread> replay_thread { };
	bool ready_signal { false };
	lfsr_word_t lfsr_v = 1;
	
	//void show_infos();
	void start_tx(const uint32_t id);
	//void on_ctcss_changed(uint32_t v);
	void stop();
	bool is_active() const;
	void set_ready();
	void handle_replay_thread_done(const uint32_t return_code);
	void file_error();
	void on_tx_progress(const uint32_t progress);
	void refresh_list();
	void on_select_entry();
	
	Labels labels {
		//{ { 0, 20 * 8 + 4 }, "Title:", Color::light_grey() },
		{ { 0, 180 }, "Key:", Color::light_grey() }
	};

	Button button_next_page {
		{ 30 * 7, 25 * 8, 10 * 3, 2 * 14 },
		"=>"
	};

	Button button_prev_page {
		{ 17 * 10, 25 * 8, 10 * 3, 2 * 14 },
		"<="
	};

	Text page_info {
		{ 0, 30 * 8 - 4, 30 * 8, 16 }
	};

	MenuView menu_view {
		{ 0, 0, 240, 175 },
		true
	};
	Text text_empty {
		{ 7 * 8, 12 * 8, 16 * 8, 16 },
		"Empty directory !",
	};
	
	/*Text text_title {
		{ 6 * 8, 20 * 8 + 4, 15 * 8, 16 }
	};*/
	
	/*Text text_duration {
		{ 22 * 8, 20 * 8 + 4, 6 * 8, 16 }
	};*/
	
	OptionsField options_tone_key {
		{ 32 , 180 },
		18,
		{ }
	};
	
	Checkbox check_loop {
		{ 0, 25 * 8 + 4 },
		4,
		"Loop"
	};
	
	Checkbox check_random {
		{ 10 * 7, 25 * 8 + 4 },
		6,
		"Random"
	};
	
	//ProgressBar progressbar {
	//	{ 0 * 8, 30 * 8 - 4, 30 * 8, 16 }
	//};
	
	TransmitterView tx_view {
		16 * 16,
		5000,
		12
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
