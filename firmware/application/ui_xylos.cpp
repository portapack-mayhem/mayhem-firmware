/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_xylos.hpp"

#include "ch.h"
#include "evtimer.h"

#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace hackrf::one;

namespace ui {

void XylosView::focus() {
	city_code.focus();
}

XylosView::~XylosView() {
	transmitter_model.disable();
}

void XylosView::paint(Painter& painter) {

}

void XylosView::upd_message() {
	uint8_t c;
		
	ccirmessage[0] = '0';
	ccirmessage[1] = '0';
	ccirmessage[2] = '0';
	ccirmessage[3] = '0';
	
	ccirmessage[4] = (city_code.value() / 10) + 0x30;
	ccirmessage[5] = (city_code.value() % 10) + 0x30;
	ccirmessage[6] = family_code.value() + 0x30;
	if (checkbox_wcsubfamily.value() == false)
		ccirmessage[7] = subfamily_code.value() + 0x30;
	else
		ccirmessage[7] = 'A';
	if (checkbox_wcid.value() == false) {
		ccirmessage[8] = (receiver_code.value() / 10) + 0x30;
		ccirmessage[9] = (receiver_code.value() % 10) + 0x30;
	} else {
		ccirmessage[8] = 'A';
		ccirmessage[9] = 'A';
	}
	
	ccirmessage[10] = 'B';
	
	ccirmessage[11] = options_ra.selected_index() + 0x30;
	ccirmessage[12] = options_rb.selected_index() + 0x30;
	ccirmessage[13] = options_rc.selected_index() + 0x30;
	ccirmessage[14] = '0';
	
	ccirmessage[15] = 'B';

	ccirmessage[16] = '0';
	ccirmessage[17] = '0';
	ccirmessage[18] = '0';
	ccirmessage[19] = '0';
	
	// Stuffing
	for (c=1; c<20; c++) {
		if (ccirmessage[c] == ccirmessage[c-1]) ccirmessage[c] = 'E';
	}
	
	ccirmessage[20] = 0;
	
	text_debug.set(ccirmessage);
}

XylosView::XylosView(
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	
	static constexpr Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	
	transmitter_model.set_modulation(19);
	
	add_children({ {
		&text_title,
		&text_city,
		&city_code,
		&text_family,
		&family_code,
		&text_subfamily,
		&subfamily_code,
		&checkbox_wcsubfamily,
		&text_receiver,
		&receiver_code,
		&checkbox_wcid,
		&text_freq,
		&options_freq,
		&text_ra,
		&options_ra,
		&text_rb,
		&options_rb,
		&text_rc,
		&options_rc,
		&text_progress,
		&text_debug,
		&button_transmit,
		&button_exit
	} });
	
	city_code.set_value(18);
	family_code.set_value(1);
	subfamily_code.set_value(1);
	receiver_code.set_value(1);
	options_freq.set_selected_index(5);
	
	checkbox_wcsubfamily.set_value(true);
	checkbox_wcid.set_value(true);
	
	city_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	family_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	subfamily_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	receiver_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	checkbox_wcsubfamily.on_select = [this](Checkbox&) {
		XylosView::upd_message();
	};
	checkbox_wcid.on_select = [this](Checkbox&) {
		XylosView::upd_message();
	};
	options_ra.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	options_rb.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	options_rc.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	
	button_transmit.set_style(&style_val);
	
	button_transmit.on_select = [this,&transmitter_model](Button&) {
		if (txing == false) {
			upd_message();
			
			auto& message_map = context().message_map();
			
			message_map.unregister_handler(Message::ID::TXDone);
			
			message_map.register_handler(Message::ID::TXDone,
				[this,&transmitter_model](Message* const p) {
					uint8_t c;
					char progress[21];
					const auto message = static_cast<const TXDoneMessage*>(p);
					if (message->n == 25) {
						transmitter_model.disable();
						for (c=0;c<20;c++)
							progress[c] = ' ';
						progress[20] = 0;
						text_progress.set(progress);
						txing = false;
						button_transmit.set_style(&style_val);
						button_transmit.set_text("START");
					} else {
						for (c=0;c<message->n;c++)
							progress[c] = ' ';
						progress[c] = '.';
						progress[++c] = 0;
						text_progress.set(progress);
					}
				}
			);
			
			shared_memory.xylos_transmit_done = false;
			memcpy(shared_memory.xylosdata, ccirmessage, 21);
			
			transmitter_model.set_tuning_frequency(87700000); //xylos_freqs[options_freq.selected_index()]);
			
			txing = true;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			transmitter_model.enable();
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
}

} /* namespace ui */
