/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_scanner.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"
#include "audio.hpp"

using namespace portapack;

namespace ui {

ScannerThread::ScannerThread(
	std::vector<rf::Frequency> frequency_list
) : frequency_list_ {  std::move(frequency_list) }
{
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ScannerThread::static_fn, this);
}

ScannerThread::~ScannerThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

void ScannerThread::set_scanning(const bool v) {
	_scanning = v;
}

msg_t ScannerThread::static_fn(void* arg) {
	auto obj = static_cast<ScannerThread*>(arg);
	obj->run();
	return 0;
}

void ScannerThread::run() {
	RetuneMessage message { };
	uint32_t frequency_index = 0;
	
	while( !chThdShouldTerminate() ) {
		if (_scanning) {
			// Retune
			receiver_model.set_tuning_frequency(frequency_list_[frequency_index]);
			
			message.range = frequency_index;
			EventDispatcher::send_message(message);
			
			
			frequency_index++;
			if (frequency_index >= frequency_list_.size())
				frequency_index = 0;
		}
		
		chThdSleepMilliseconds(50);
	}
}

void ScannerView::handle_retune(uint32_t i) {
	text_cycle.set(	to_string_dec_uint(i) + "/" +
					to_string_dec_uint(frequency_list.size()) + " : " +
					to_string_dec_uint(frequency_list[i]) );
	desc_cycle.set( description_list[i] );
}

void ScannerView::focus() {
	field_lna.focus();
}

ScannerView::~ScannerView() {
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

ScannerView::ScannerView(
	NavigationView&
)
{
	add_children({
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&field_volume,
		&field_bw,
		&field_trigger,
		&field_squelch,
		&field_wait,
		//&record_view,
		&text_cycle,
		&desc_cycle,
		//&waterfall,
	});

	std::string scanner_file = "SCANNER";
	if (load_freqman_file(scanner_file, database)) {
		for(auto& entry : database) {
			// FIXME
			if (entry.type == RANGE) {
				for (uint32_t i=entry.frequency_a; i < entry.frequency_b; i+= 1000000) {
					frequency_list.push_back(i);
					description_list.push_back("RNG " + to_string_dec_uint(entry.frequency_a) + ">" + to_string_dec_uint(entry.frequency_b));
				}
			} else {
				frequency_list.push_back(entry.frequency_a);
				description_list.push_back(entry.description);
			}
		}
	} else {
		// DEBUG
		// TODO: Clean this
		frequency_list.push_back(466025000);
		description_list.push_back("POCSAG-France");
		frequency_list.push_back(466050000);
		description_list.push_back("POCSAG-France");
		frequency_list.push_back(466075000);
		description_list.push_back("POCSAG-France");
		frequency_list.push_back(466175000);
		description_list.push_back("POCSAG-France");
		frequency_list.push_back(466206250);
		description_list.push_back("POCSAG-France");
		frequency_list.push_back(466231250);
		description_list.push_back("POCSAG-France");
	}

	field_bw.set_selected_index(2);
	field_bw.on_change = [this](size_t n, OptionsField::value_t) {
		receiver_model.set_nbfm_configuration(n);
	};

	field_wait.on_change = [this](int32_t v) {
		wait = v;
	};
	field_wait.set_value(5);

	field_trigger.on_change = [this](int32_t v) {
		trigger = v;
	};
	field_trigger.set_value(30);
	
	field_squelch.set_value(receiver_model.squelch_level());
	field_squelch.on_change = [this](int32_t v) {
		squelch = v;
		receiver_model.set_squelch_level(v);
	};


	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};
	
	audio::output::start();
	
	audio::output::mute();
	baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
	receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.enable();
	receiver_model.set_squelch_level(0);
	receiver_model.set_nbfm_configuration(field_bw.selected_index());
	audio::output::unmute();
	
	// TODO: Scanning thread here
	scan_thread = std::make_unique<ScannerThread>(frequency_list);
}

void ScannerView::on_statistics_update(const ChannelStatistics& statistics) {
	int32_t max_db = statistics.max_db;
	
	if (timer <= wait)
		timer++;
	
	if (max_db < -trigger) {
		if (timer == wait) {
			//audio::output::stop();
			scan_thread->set_scanning(true);
		}
	} else {
		//audio::output::start();
		scan_thread->set_scanning(false);
		timer = 0;
	}
}

void ScannerView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

} /* namespace ui */
