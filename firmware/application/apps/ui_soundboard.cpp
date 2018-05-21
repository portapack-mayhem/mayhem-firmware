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

// To prepare samples: for f in ./*.wav; do sox "$f" -r 48000 -c 1 -b8 --norm "conv/$f"; done

#include "ui_soundboard.hpp"
#include "lfsr_random.hpp"
#include "string_format.hpp"
#include "tonesets.hpp"

using namespace tonekey;
using namespace portapack;

namespace ui {

// TODO: Use Sharebrained's PRNG
void SoundBoardView::do_random() {
	uint32_t id;
	
	chThdSleepMilliseconds(500);
	
	lfsr_v = lfsr_iterate(lfsr_v);
	id = lfsr_v % max_sound;

	play_sound(id);
	
	buttons[id % 15].focus();
	page = id / 15;
	
	refresh_buttons(id);
}

bool SoundBoardView::is_active() const {
	return (bool)replay_thread;
}

void SoundBoardView::stop(const bool do_loop) {
	if( is_active() )
		replay_thread.reset();
	
	if (do_loop && check_loop.value()) {
		play_sound(playing_id);
	} else {
		radio::disable();
		//button_play.set_bitmap(&bitmap_play);
	}
	ready_signal = false;
}

void SoundBoardView::handle_replay_thread_done(const uint32_t return_code) {
	if (return_code == ReplayThread::END_OF_FILE) {
		stop(true);
	} else if (return_code == ReplayThread::READ_ERROR) {
		stop(false);
		file_error();
	}
	
	progressbar.set_value(0);
}

void SoundBoardView::set_ready() {
	ready_signal = true;
}

void SoundBoardView::focus() {
	buttons[0].focus();

	if (!max_sound)
		nav_.display_modal("No files", "No files in /WAV/ directory", ABORT, nullptr);
}

void SoundBoardView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void SoundBoardView::file_error() {
	nav_.display_modal("Error", "File read error.");
}

void SoundBoardView::play_sound(uint16_t id) {
	uint32_t sample_rate = 0;
	auto reader = std::make_unique<WAVFileReader>();
	uint32_t tone_key_index = options_tone_key.selected_index();
	
	stop(false);

	if (!reader->open(sounds[id].path)) {
		file_error();
		return;
	}
	
	playing_id = id;
	
	progressbar.set_max(reader->sample_count());
	sample_rate = reader->sample_rate() * 32;
	
	if( reader ) {
		//button_play.set_bitmap(&bitmap_stop);
		baseband::set_sample_rate(sample_rate);
		
		replay_thread = std::make_unique<ReplayThread>(
			std::move(reader),
			read_size, buffer_count,
			&ready_signal,
			[](uint32_t return_code) {
				ReplayThreadDoneMessage message { return_code };
				EventDispatcher::send_message(message);
			}
		);
	}
	
	baseband::set_audiotx_config(
		0,	// Divider is unused
		number_bw.value() * 1000,
		0,	// Gain is unused
		TONES_F2D(tone_key_frequency(tone_key_index), sample_rate)
	);
	
	radio::enable({
		receiver_model.tuning_frequency(),
		sample_rate,
		1750000,
		rf::Direction::Transmit,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga())
	});
}

void SoundBoardView::show_infos(uint16_t id) {
	text_duration.set(to_string_time_ms(sounds[id].ms_duration));
	
	text_title.set(sounds[id].title);
}

void SoundBoardView::refresh_buttons(uint16_t id) {
	size_t n = 0, n_sound;
	
	text_page.set("Page " + to_string_dec_uint(page + 1) + "/" + to_string_dec_uint(max_page));
	
	for (auto& button : buttons) {
		n_sound = (page * 15) + n;
		
		button.id = n_sound;
		
		if (n_sound < max_sound) {
			button.set_text(sounds[n_sound].path.stem().string().substr(0, 8));
			button.set_style(styles[sounds[n_sound].path.stem().string()[0] & 3]);
		} else {
			button.set_text("- - -");
			button.set_style(styles[0]);
		}
		
		n++;
	}
	
	show_infos(id);
}

bool SoundBoardView::change_page(Button& button, const KeyEvent key) {
	// Stupid way to find out if the button is on the sides
	if (button.screen_pos().x() < 32) {
		if ((key == KeyEvent::Left) && (page > 0)) {
			page--;
			refresh_buttons(button.id);
			return true;
		}
	} else if (button.screen_pos().x() > 120) {
		if ((key == KeyEvent::Right) && (page < max_page - 1)) {
			page++;
			refresh_buttons(button.id);
			return true;
		}
	}
	return false;
}

void SoundBoardView::on_tx_progress(const uint32_t progress) {
	progressbar.set_value(progress);
}

SoundBoardView::SoundBoardView(
	NavigationView& nav
) : nav_ (nav)
{
	auto reader = std::make_unique<WAVFileReader>();
	uint8_t c = 0;
	
	for(const auto& entry : std::filesystem::directory_iterator(u"WAV", u"*.WAV")) {
		if( std::filesystem::is_regular_file(entry.status()) ) {
			if (reader->open(u"WAV/" + entry.path().native())) {
				if ((reader->channels() == 1) && (reader->bits_per_sample() == 8)) {
					sounds[c].ms_duration = reader->ms_duration();
					sounds[c].path = u"WAV/" + entry.path().native();
					std::string title = reader->title().substr(0, 20);
					if (title != "")
						sounds[c].title = title;
					else
						sounds[c].title = "-";
					c++;
					if (c == 60) break;		// Limit to 60 files (4 pages)
				}
			}
		}
	}
	
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
	
	max_sound = c;
	max_page = (max_sound + 15 - 1) / 15;	// 3 * 5 = 15 buttons per page
	
	add_children({
		&labels,
		&field_frequency,
		&number_bw,
		&options_tone_key,
		&text_title,
		&text_page,
		&text_duration,
		&progressbar,
		&check_loop,
		&button_random,
		&button_exit
	});
	
	tone_keys_populate(options_tone_key);
	options_tone_key.set_selected_index(0);
	
	const auto button_fn = [this](Button& button) {
		tx_mode = NORMAL;
		this->play_sound(button.id);
	};
	
	const auto button_focus = [this](Button& button) {
		this->show_infos(button.id);
	};
	
	const auto button_dir = [this](Button& button, const KeyEvent key) {
		return change_page(button, key);
	};
	
	// Generate buttons
	size_t n = 0;
	for(auto& button : buttons) {
		add_child(&button);
		button.on_select = button_fn;
		button.on_highlight = button_focus;
		button.on_dir = button_dir;
		button.set_parent_rect({
			static_cast<Coord>((n % 3) * 78 + 3),
			static_cast<Coord>((n / 3) * 38 + 24),
			78, 38
		});
		n++;
	}
	refresh_buttons(0);
	
	check_loop.set_value(false);
	number_bw.set_value(12);

	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(10000);
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};
	
	button_random.on_select = [this](Button&){
		if (tx_mode == NORMAL) {
			tx_mode = RANDOM;
			button_random.set_text("STOP");
			do_random();
		} else {
			tx_mode = NORMAL;
			button_random.set_text("Random");
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

SoundBoardView::~SoundBoardView() {
	transmitter_model.disable();
	baseband::shutdown();
}

}
