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

#include "soundboard_app.hpp"
#include "string_format.hpp"
#include "tonesets.hpp"

using namespace tonekey;
using namespace portapack;

namespace ui {

bool SoundBoardView::is_active() const {
	return (bool)replay_thread;
}

void SoundBoardView::stop() {
	if (is_active())
		replay_thread.reset();
	
	transmitter_model.disable();
	tx_view.set_transmitting(false);
	
	//button_play.set_bitmap(&bitmap_play);
	ready_signal = false;
}

void SoundBoardView::handle_replay_thread_done(const uint32_t return_code) {
	stop();
	//progressbar.set_value(0);
	
	if (return_code == ReplayThread::END_OF_FILE) {
		if (check_random.value()) {
			lfsr_v = lfsr_iterate(lfsr_v);
			playing_id = lfsr_v % file_list.size();
			menu_view.set_highlighted(playing_id);
			start_tx(playing_id);
		} else if (check_loop.value()) {
			start_tx(playing_id);
		}
	} else if (return_code == ReplayThread::READ_ERROR) {
		file_error();
	}
}

void SoundBoardView::set_ready() {
	ready_signal = true;
}

void SoundBoardView::focus() {
	menu_view.focus();
}

void SoundBoardView::file_error() {
	nav_.display_modal("Error", "File read error.");
}

void SoundBoardView::start_tx(const uint32_t id) {
	auto reader = std::make_unique<WAVFileReader>();
	uint32_t tone_key_index = options_tone_key.selected_index();
	uint32_t sample_rate;
	
	stop();

	if (!reader->open(u"/WAV/" + file_list[id].native())) {
		file_error();
		return;
	}
	
	playing_id = id;
	
	//progressbar.set_max(reader->sample_count());
	
	//button_play.set_bitmap(&bitmap_stop);
	
	sample_rate = reader->sample_rate();
	
	replay_thread = std::make_unique<ReplayThread>(
		std::move(reader),
		read_size, buffer_count,
		&ready_signal,
		[](uint32_t return_code) {
			ReplayThreadDoneMessage message { return_code };
			EventDispatcher::send_message(message);
		}
	);
	
	baseband::set_audiotx_config(
		1536000 / 20,		// Update vu-meter at 20Hz
		transmitter_model.channel_bandwidth(),
		0,	// Gain is unused
		TONES_F2D(tone_key_frequency(tone_key_index), 1536000),
		0, //AM
		0, //DSB
		0, //USB
		0 //LSB
	);
	baseband::set_sample_rate(sample_rate);
	
	transmitter_model.set_sampling_rate(1536000);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	tx_view.set_transmitting(true);
}

/*void SoundBoardView::show_infos() {
	if (!reader->open(file_list[menu_view.highlighted_index()]))
		return;
	
	text_duration.set(to_string_time_ms(reader->ms_duration()));
	text_title.set(reader->title().substr(0, 15));
}*/

void SoundBoardView::on_tx_progress(const uint32_t progress) {
	(void)progress ; // avoid warning
	//progressbar.set_value(progress);
}

void SoundBoardView::on_select_entry() {
	tx_view.focus();
}

void SoundBoardView::refresh_list() {
	auto reader = std::make_unique<WAVFileReader>();
	
	file_list.clear();
	c_page = page;
	
	// List directories and files, put directories up top
	uint32_t count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(u"WAV", u"*")) {
		if (std::filesystem::is_regular_file(entry.status())) {
			if (entry.path().string().length()) {
			
				auto entry_extension = entry.path().extension().string();
			
				for (auto &c: entry_extension)
					c = toupper(c);
				
				if (entry_extension == ".WAV") {
					
					if (reader->open(u"/WAV/" + entry.path().native())) {
						if ((reader->channels() == 1) && (reader->bits_per_sample() == 8)) {
							//sounds[c].ms_duration = reader->ms_duration();
							//sounds[c].path = u"WAV/" + entry.path().native();
							if (count >= (page - 1) * 100 && count < page * 100){
								file_list.push_back(entry.path());
								if (file_list.size() == 100){
									page++;
									break;
								}
							}
							count++;
						}
					}
				}
			}
		}
	}

	if (!file_list.size()) {
		// Hide widgets, show warning
		if (page == 1){
			menu_view.hidden(true);
			text_empty.hidden(false);
			set_dirty();
		}else{
			page = 1;
			refresh_list();
			return;
		}
	} else {
		// Hide warning, show widgets
		menu_view.hidden(false);
		text_empty.hidden(true);
		set_dirty();
	
		menu_view.clear();
		
		for (size_t n = 0; n < file_list.size(); n++) {
			menu_view.add_item({
				file_list[n].string().substr(0, 30),
				ui::Color::white(),
				nullptr,
				[this](){
					on_select_entry();
				}
			});
		}

		page_info.set("Page: " + to_string_dec_uint(c_page) + "    Sounds: " + to_string_dec_uint(file_list.size()));
		menu_view.set_highlighted(0);	// Refresh
	}

	if (file_list.size() < 100){
		page = 1;
	}
}

SoundBoardView::SoundBoardView(
	NavigationView& nav
) : nav_ (nav)
{
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
	
	add_children({
		&labels,
		&menu_view,
		&text_empty,
		&options_tone_key,
		//&text_title,
		//&text_duration,
		//&progressbar,
		&page_info,
		&check_loop,
		&check_random,
		&button_prev_page,
		&button_next_page,
		&tx_view
	});

	// load app settings
	auto rc = settings.load("tx_soundboard", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_channel_bandwidth(app_settings.channel_bandwidth);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}
	
	refresh_list();

	button_next_page.on_select = [this](Button&) {
		this->refresh_list();
	};

	button_prev_page.on_select = [this](Button&) {
		if (c_page == 1) return;
		if (c_page == 2) page = 1;
		page = c_page - 1;
		refresh_list();
	};
	
	//text_title.set(to_string_dec_uint(file_list.size()));
	
	tone_keys_populate(options_tone_key);
	options_tone_key.set_selected_index(0);
	
	check_loop.set_value(false);
	check_random.set_value(false);

	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			transmitter_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		start_tx(menu_view.highlighted_index());
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		stop();
	};
}

SoundBoardView::~SoundBoardView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	settings.save("tx_soundboard", &app_settings);

	stop();
	transmitter_model.disable();
	baseband::shutdown();
}

}
