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

using namespace ctcss;
using namespace portapack;

namespace ui {

void SoundBoardView::do_random() {
	uint32_t id;
	
	chThdSleepMilliseconds(500);
	
	lfsr_v = lfsr_iterate(lfsr_v);
	id = lfsr_v % max_sound;

	play_sound(id);
	
	buttons[id % 18].focus();
	page = id / 18;
	
	refresh_buttons(id);
}

void SoundBoardView::prepare_audio() {
	
	if (sample_counter >= sample_duration) {
		if (tx_mode == NORMAL) {
			if (!check_loop.value()) {
				pbar.set_value(0);
				transmitter_model.disable();
				return;
			} else {
				reader->rewind();
				sample_counter = 0;
			}
		} else {
			pbar.set_value(0);
			transmitter_model.disable();
			if (check_loop.value())
				do_random();
		}
	}
	
	pbar.set_value(sample_counter);

	auto bytes_read = reader->read(audio_buffer, 512).value();
	
	// Unsigned to signed, pretty stupid :/
	for (size_t n = 0; n < bytes_read; n++)
		audio_buffer[n] -= 0x80;
	for (size_t n = bytes_read; n < 512; n++)
		audio_buffer[n] = 0;
	
	sample_counter += 512;
	
	baseband::set_fifo_data(audio_buffer);
}

void SoundBoardView::focus() {
	buttons[0].focus();

	if (!max_sound)
		nav_.display_modal("No files", "No files in /WAV/ directory", ABORT, nullptr);
}

void SoundBoardView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void SoundBoardView::play_sound(uint16_t id) {
	uint32_t ctcss_index;
	bool ctcss_enabled;
	uint32_t divider;

	if (sounds[id].size == 0) return;

	if (!reader->open(sounds[id].path)) return;
	
	sample_duration = sounds[id].sample_duration;
	
	pbar.set_max(sample_duration);
	pbar.set_value(0);
	
	sample_counter = 0;
	
	prepare_audio();
	
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	ctcss_index = options_ctcss.selected_index();
	
	if (ctcss_index) {
		ctcss_enabled = true;
		ctcss_index--;
	} else
		ctcss_enabled = false;
	
	divider = (1536000 / sounds[id].sample_rate) - 1;
	
	baseband::set_audiotx_data(
		divider,
		number_bw.value() * 1000,
		1,
		ctcss_enabled,
		(uint32_t)((ctcss_tones[ctcss_index].frequency / 1536000.0) * 0xFFFFFFFFULL)
	);
}

void SoundBoardView::show_infos(uint16_t id) {
	uint32_t duration = sounds[id].ms_duration;
	
	text_duration.set(to_string_dec_uint(duration / 1000) + "." + to_string_dec_uint((duration / 100) % 10) + "s");
	
	text_title.set(sounds[id].title);
}

void SoundBoardView::refresh_buttons(uint16_t id) {
	size_t n = 0, n_sound;
	
	text_page.set("Page " + to_string_dec_uint(page + 1) + "/" + to_string_dec_uint(max_page));
	
	for (auto& button : buttons) {
		n_sound = (page * 18) + n;
		
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

void SoundBoardView::change_page(Button& button, const KeyEvent key) {
	// Stupid way to find out if the button is on the sides
	if (button.screen_pos().x() < 32) {
		if ((key == KeyEvent::Left) && (page > 0)) {
			page--;
			refresh_buttons(button.id);
		}
	} else if (button.screen_pos().x() > 120) {
		if ((key == KeyEvent::Right) && (page < max_page - 1)) {
			page++;
			refresh_buttons(button.id);
		}
	}
}

SoundBoardView::SoundBoardView(
	NavigationView& nav
) : nav_ (nav)
{
	std::vector<std::filesystem::path> file_list;
	std::string title;
	uint8_t c;
	
	reader = std::make_unique<WAVFileReader>();
	
	file_list = scan_root_files(u"WAV", u"*.WAV");
	
	c = 0;
	for (auto& path : file_list) {
		if (reader->open(u"WAV/" + path.native())) {
			if ((reader->channels() == 1) && (reader->bits_per_sample() == 8)) {
				sounds[c].size = reader->data_size();
				sounds[c].sample_duration = reader->data_size(); // / (reader->bits_per_sample() / 8);
				sounds[c].sample_rate = reader->sample_rate();
				//if (reader->bits_per_sample() > 8)
				//	sounds[c].sixteenbit = true;
				//else
				//	sounds[c].sixteenbit = false;
				sounds[c].ms_duration = reader->ms_duration();
				sounds[c].path = u"WAV/" + path.native();
				title = reader->title().substr(0, 20);
				if (title != "")
					sounds[c].title = title;
				else
					sounds[c].title = "-";
				c++;
				if (c == 108) break;		// Limit to 108 files (6 pages)
			}
		}
	}
	
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);

	max_sound = c;
	max_page = (max_sound + 18 - 1) / 18;	// 3 * 6 = 18 buttons per page
	
	add_children({
		&field_frequency,
		&number_bw,
		&text_kHz,
		&options_ctcss,
		&text_title,
		&text_page,
		&text_duration,
		&pbar,
		&check_loop,
		&button_random,
		&button_exit
	});

	ctcss_populate(options_ctcss);
	options_ctcss.set_selected_index(0);

	const auto button_fn = [this](Button& button) {
		tx_mode = NORMAL;
		this->play_sound(button.id);
	};
	
	const auto button_focus = [this](Button& button) {
		this->show_infos(button.id);
	};
	
	const auto button_dir = [this](Button& button, const KeyEvent key) {
		this->change_page(button, key);
		return false;
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
			static_cast<Coord>((n / 3) * 30 + 24),
			78, 30
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
