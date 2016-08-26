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

#include "ch.h"
#include "file.hpp"

#include "ui_alphanum.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "event_m0.hpp"
#include "string_format.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>
#include <vector>

using namespace portapack;

namespace ui {

uint16_t SoundBoardView::shitty_rand() {
	uint8_t bit;
	
	bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
	
	return lfsr = (lfsr >> 1) | (bit << 15);
}

void SoundBoardView::do_random() {
	uint16_t id;
	
	chThdSleepMilliseconds(300);	// 100ms
	
	id = shitty_rand() % max_sound;

	play_sound(id);
	
	buttons[id % 21].focus();
	page = id / 21;
	
	refresh_buttons(id);
}

void SoundBoardView::prepare_audio() {
	
	if (cnt >= sample_duration) {
		if (tx_mode == NORMAL) {
			if (!check_loop.value()) {
				pbar.set_value(0);
				transmitter_model.disable();
				return;
			} else {
				file.seek(44);
				cnt = 0;
			}
		} else {
			pbar.set_value(0);
			transmitter_model.disable();
			do_random();
		}
	}
	
	pbar.set_value(cnt);
	
	file.read(audio_buffer, 1024);
	
	for (size_t n = 0; n < 1024; n++)
		audio_buffer[n] -= 0x80;
	
	cnt += 1024;
	
	baseband::set_fifo_data(audio_buffer);
}

void SoundBoardView::focus() {
	buttons[0].focus();
}

void SoundBoardView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void SoundBoardView::play_sound(uint16_t id) {

	if (sounds[id].size == 0) return;

	auto error = file.open("/wav/" + sounds[id].filename);
	if (error.is_valid()) return;
	
	sample_duration = sounds[id].sample_duration;
	
	pbar.set_max(sample_duration);
	pbar.set_value(0);
	
	cnt = 0;
	file.seek(44);	// Skip header
	
	prepare_audio();
	
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 1536000,
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_audiotx_data(1536000 / sounds[id].sample_rate, number_bw.value());
}

void SoundBoardView::show_infos(uint16_t id) {
	uint32_t duration = sounds[id].ms_duration;
	
	text_duration.set(to_string_dec_uint(duration / 1000) + "." + to_string_dec_uint((duration / 100) % 10) + "s");
}

void SoundBoardView::refresh_buttons(uint16_t id) {
	size_t n = 0, n_sound;
	
	text_page.set(to_string_dec_uint(page + 1) + "/" + to_string_dec_uint(max_page));
	
	for (auto& button : buttons) {
		n_sound = (page * 21) + n;
		
		button.id = n_sound;
		
		if (n_sound < max_sound) {
			button.set_text(sounds[n_sound].shortname);
			button.set_style(styles[sounds[n_sound].shortname[0] & 3]);
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
	if (button.screen_pos().x < 32) {
		if ((key == KeyEvent::Left) && (page > 0)) {
			page--;
			refresh_buttons(button.id);
		}
	}
	if (button.screen_pos().x > 120) {
		if ((key == KeyEvent::Right) && (page < max_page - 1)) {
			page++;
			refresh_buttons(button.id);
		}
	}
}

uint16_t SoundBoardView::fb_to_uint16(const std::string& fb) {
	return (fb[1] << 8) + fb[0];
}

uint32_t SoundBoardView::fb_to_uint32(const std::string& fb) {
	return (fb[3] << 24) + (fb[2] << 16) + (fb[1] << 8) + fb[0];
}

SoundBoardView::SoundBoardView(
	NavigationView& nav
)
{
	std::vector<std::string> file_list;
	std::string file_name;
	uint32_t size;
	uint8_t c;
	
	char file_buffer[32];
	
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);

	file_list = scan_root_files("/wav", ".WAV");

	c = 0;
	for (auto& file_name : file_list) {
		
		auto error = file.open("/wav/" + file_name);
		
		file.seek(40);
		file.read(file_buffer, 4);
		size = fb_to_uint32(file_buffer);
		sounds[c].size = size;
		
		file.seek(22);
		file.read(file_buffer, 2);
		if (fb_to_uint16(file_buffer) > 1) {
			sounds[c].stereo = true;
			size /= 2;
		} else
			sounds[c].stereo = false;
		
		file.seek(24);
		file.read(file_buffer, 4);
		sounds[c].sample_rate = fb_to_uint32(file_buffer);
		
		file.seek(34);
		file.read(file_buffer, 2);
		if (fb_to_uint16(file_buffer) > 8) {
			sounds[c].sixteenbit = true;
			size /= 2;
		} else
			sounds[c].sixteenbit = false;
		
		sounds[c].ms_duration = (size * 1000) / sounds[c].sample_rate;
		sounds[c].sample_duration = size;
		
		sounds[c].filename = file_name;
		sounds[c].shortname = remove_filename_extension(file_name);
		
		c++;
		if (c == 100) break;
	}
	
	max_sound = c;
	max_page = max_sound / 21;
	
	add_children({ {
		&field_frequency,
		&number_bw,
		&text_kHz,
		&text_page,
		&text_duration,
		&pbar,
		&check_loop,
		&button_random,
		&button_exit
	} });

	const auto button_fn = [this](Button& button) {
		tx_mode = NORMAL;
		this->play_sound(button.id);
	};
	
	const auto button_focus = [this](Button& button) {
		this->show_infos(button.id);
	};
	
	const auto button_dir = [this](Button& button, const KeyEvent key) {
		this->change_page(button, key);
	};

	size_t n = 0;
	for(auto& button : buttons) {
		add_child(&button);
		button.on_select = button_fn;
		button.on_highlight = button_focus;
		button.on_dir = button_dir;
		button.set_parent_rect({
			static_cast<Coord>((n % 3) * 78 + 3),
			static_cast<Coord>((n / 3) * 28 + 26),
			78, 28
		});
		n++;
	}
	refresh_buttons(0);
	
	check_loop.set_value(false);
	number_bw.set_value(15);

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
