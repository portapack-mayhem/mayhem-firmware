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

#include "ui_rds.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;
using namespace rds;

namespace ui {

RDSPSNView::RDSPSNView(
	NavigationView& nav, 
	Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("PSN");
	
	add_children({
		&labels,
		&text_psn,
		&button_set,
		&check_mono_stereo,
		&check_TA,
		&check_MS
	});
	
	set_enabled(true);
	
	check_TA.set_value(true);
	
	check_mono_stereo.on_select = [this](Checkbox&, bool value) {
		mono_stereo = value;
	};
	check_TA.on_select = [this](Checkbox&, bool value) {
		TA = value;
	};
	check_MS.on_select = [this](Checkbox&, bool value) {
		MS = value;
	};
	
	button_set.on_select = [this, &nav](Button&) {
		text_prompt(
			nav,
			PSN,
			8,
			[this](std::string& s) {
				text_psn.set(s);
			}
		);
	};
}

RDSRadioTextView::RDSRadioTextView(
	NavigationView& nav, 
	Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("Radiotext");
	
	add_children({
		&labels,
		&button_set,
		&text_radiotext
	});
	
	button_set.on_select = [this, &nav](Button&){
		text_prompt(
			nav,
			radiotext,
			28,
			[this](std::string& s) {
				text_radiotext.set(s);
			}
		);
	};
}

RDSDateTimeView::RDSDateTimeView(
	Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("date & time");
	
	add_children({
		&labels
	});
}

RDSAudioView::RDSAudioView(
	Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("audio");
	
	add_children({
		&labels
	});
}

RDSThread::RDSThread(
	std::vector<RDSGroup>** frames
) : frames_ { std::move(frames) }
{
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, RDSThread::static_fn, this);
}

RDSThread::~RDSThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

msg_t RDSThread::static_fn(void* arg) {
	auto obj = static_cast<RDSThread*>(arg);
	obj->run();
	return 0;
}

void RDSThread::run() {
	std::vector<RDSGroup>* frame_ptr;
	size_t block_count, c;
	uint32_t * tx_data_u32 = (uint32_t*)shared_memory.bb_data.data;
	uint32_t frame_index = 0;
	
	while( !chThdShouldTerminate() ) {

		do {
			frame_ptr = frames_[frame_index];
			
			if (frame_index == 2) {
				frame_index = 0;
			} else {
				frame_index++;
			}
		} while(!(block_count = frame_ptr->size() * 4));
		
		for (c = 0; c < block_count; c++)
			tx_data_u32[c] = frame_ptr->at(c >> 2).block[c & 3];
	
		baseband::set_rds_data(block_count * 26);
		
		chThdSleepMilliseconds(1000);
	}
}

void RDSView::focus() {
	tab_view.focus();
}

RDSView::~RDSView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void RDSView::start_tx() {
	rds_flags.PI_code = sym_pi_code.value_hex_u64();
	rds_flags.PTY = options_pty.selected_index_value();
	rds_flags.DI = view_PSN.mono_stereo ? 1 : 0;
	rds_flags.TP = check_TP.value();
	rds_flags.TA = view_PSN.TA;
	rds_flags.MS = view_PSN.MS;
	
	if (view_PSN.is_enabled())
		gen_PSN(frame_psn, view_PSN.PSN, &rds_flags);
	else
		frame_psn.clear();
	
	if (view_radiotext.is_enabled())
		gen_RadioText(frame_radiotext, view_radiotext.radiotext, 0, &rds_flags);
	else
		frame_radiotext.clear();
	
	// DEBUG
	if (view_datetime.is_enabled())
		gen_ClockTime(frame_datetime, &rds_flags, 2016, 12, 1, 9, 23, 2);
	else
		frame_datetime.clear();
	
	transmitter_model.set_sampling_rate(2280000U);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	tx_thread = std::make_unique<RDSThread>(frames);
}

RDSView::RDSView(
	NavigationView& nav
) : nav_ { nav }
{
	baseband::run_image(portapack::spi_flash::image_tag_rds);
	
	add_children({
		&tab_view,
		&labels,
		&sym_pi_code,
		&check_TP,
		&options_pty,
		&view_PSN,
		&view_radiotext,
		&view_datetime,
		&view_audio,
		//&options_countrycode,
		//&options_coverage,
		&tx_view,
	});
	
	check_TP.set_value(true);
	
	sym_pi_code.set_sym(0, 0xF);
	sym_pi_code.set_sym(1, 0x3);
	sym_pi_code.set_sym(2, 0xE);
	sym_pi_code.set_sym(3, 0x0);
	sym_pi_code.on_change = [this]() {
		rds_flags.PI_code = sym_pi_code.value_hex_u64();
	};
	
	options_pty.set_selected_index(0);				// None
	//options_countrycode.set_selected_index(18);		// Baguette du fromage
	//options_coverage.set_selected_index(0);			// Local
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
		txing = true;
	};
	
	tx_view.on_stop = [this]() {
		// Kill tx_thread here ?
		tx_view.set_transmitting(false);
		transmitter_model.disable();
		txing = false;
	};
}

} /* namespace ui */
