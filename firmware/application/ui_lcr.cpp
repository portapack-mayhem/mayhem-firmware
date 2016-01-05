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

#include "ui_rds.hpp"
#include "ui_lcr.hpp"
#include "ui_receiver.hpp"
#include "ui_afsksetup.hpp"
#include "ui_debug.hpp"

#include "ch.h"

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

void LCRView::focus() {
	button_setrgsb.focus();
}

LCRView::~LCRView() {
	transmitter_model.disable();
}

void LCRView::make_frame() {
	char eom[3] = { 3, 0, 0 };	// EOM and space for checksum
	uint8_t i, pm;
	uint16_t dp;
	uint8_t cp, pp, cur_byte, new_byte;
	
	// Pad litterals right to 7 chars (not required ?)
	for (i = 0; i < 5; i++) {
		while (strlen(litteral[i]) < 7) {
			strcat(litteral[i], " ");
		}
	}
	
	// Make LCR frame
	memset(lcrframe, 0, 256);
	lcrframe[0] = 127;			// Modem sync
	lcrframe[1] = 127;
	lcrframe[2] = 127;
	lcrframe[3] = 127;
	lcrframe[4] = 127;
	lcrframe[5] = 127;
	lcrframe[6] = 127;
	lcrframe[7] = 15;			// SOM
	strcat(lcrframe, rgsb);
	strcat(lcrframe, "PA ");
	if (checkbox_am_a.value() == true) {
		strcat(lcrframe, "AM=1 AF=\"");
		strcat(lcrframe, litteral[0]);
		strcat(lcrframe, "\" CL=0 ");
	}
	if (checkbox_am_b.value() == true) {
		strcat(lcrframe, "AM=2 AF=\"");
		strcat(lcrframe, litteral[1]);
		strcat(lcrframe, "\" CL=0 ");
	}
	if (checkbox_am_c.value() == true) {
		strcat(lcrframe, "AM=3 AF=\"");
		strcat(lcrframe, litteral[2]);
		strcat(lcrframe, "\" CL=0 ");
	}
	if (checkbox_am_d.value() == true) {
		strcat(lcrframe, "AM=4 AF=\"");
		strcat(lcrframe, litteral[3]);
		strcat(lcrframe, "\" CL=0 ");
	}
	if (checkbox_am_e.value() == true) {
		strcat(lcrframe, "AM=5 AF=\"");
		strcat(lcrframe, litteral[4]);
		strcat(lcrframe, "\" CL=0 ");
	}
	strcat(lcrframe, "EC=A SAB=0");
	
	memcpy(lcrstring, lcrframe, 256);
	
	//Checksum
	checksum = 0;
	i = 7;
	while (lcrframe[i]) {
		checksum ^= lcrframe[i];
		i++;
	}
	checksum ^= eom[0];			// EOM
	checksum &= 0x7F;
	eom[1] = checksum;
	
	strcat(lcrframe, eom);
	
	//if (persistent_memory::afsk_config() & 2)
		pm = 0; // Even parity
	//else
	//	pm = 1; // Odd parity

	//if (persistent_memory::afsk_config() & 1) {
		// LSB first
		for (dp=0;dp<strlen(lcrframe);dp++) {
			pp = pm;
			new_byte = 0;
			cur_byte = lcrframe[dp];
			for (cp=0;cp<7;cp++) {
				if ((cur_byte>>cp)&1) pp++;
				new_byte |= (((cur_byte>>cp)&1)<<(7-cp));
			}
			lcrframe_f[dp] = new_byte|(pp&1);
		}
	/*} else {
		// MSB first
		for (dp=0;dp<strlen(lcrframe);dp++) {
			pp = pm;
			cur_byte = lcrframe[dp];
			for (cp=0;cp<7;cp++) {
				if ((cur_byte>>cp)&1) pp++;
			}
			lcrframe_f[dp] = (cur_byte<<1)|(pp&1);
		}
	}*/
	
	lcrframe_f[dp] = 0;
}

void LCRView::paint(Painter& painter) {
	uint8_t i;
	
	static constexpr Style style_orange {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::orange(),
	};
	
	Point offset = {
		static_cast<Coord>(104),
		static_cast<Coord>(72)
	};
	
	for (i = 0; i < 5; i++) {
		painter.draw_string(
			screen_pos() + offset,
			style_orange,
			litteral[i]
		);
		offset.y += 40;
	}
}

LCRView::LCRView(
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	char finalstr[24] = {0};
	
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	
	transmitter_model.set_modulation(TX_LCR);
	transmitter_model.set_tuning_frequency(portapack::persistent_memory::tuned_frequency());
	memset(litteral, 0, 5*8);
	memset(rgsb, 0, 5);
	
	strcpy(rgsb, RGSB_list[29]);
	button_setrgsb.set_text(rgsb);
	
	add_children({ {
		&text_recap,
		&button_setrgsb,
		&button_txsetup,
		&checkbox_am_a,
		&button_setam_a,
		&checkbox_am_b,
		&button_setam_b,
		&checkbox_am_c,
		&button_setam_c,
		&checkbox_am_d,
		&button_setam_d,
		&checkbox_am_e,
		&button_setam_e,
		&text_status,
		&button_lcrdebug,
		&button_transmit,
		&button_transmit_scan,
		&button_exit
	} });
	
	checkbox_am_a.set_value(true);
	checkbox_am_b.set_value(true);
	checkbox_am_c.set_value(true);
	checkbox_am_d.set_value(true);
	checkbox_am_e.set_value(true);
	
	// Recap: tx freq @ bps
	auto fstr = to_string_dec_int(portapack::persistent_memory::tuned_frequency() / 1000, 6);
	auto bstr = to_string_dec_int(portapack::persistent_memory::afsk_bitrate(), 4);

	strcat(finalstr, fstr.c_str());
	strcat(finalstr, " @ ");
	strcat(finalstr, bstr.c_str());
	strcat(finalstr, "bps");

	text_recap.set(finalstr);
	
	button_transmit.set_style(&style_val);
	button_transmit_scan.set_style(&style_val);
	
	button_setrgsb.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, rgsb, 4 };
		an_view->on_changed = [this](char *rgsb) {
			button_setrgsb.set_text(rgsb);
		};
		nav.push(an_view);
	};
	
	button_setam_a.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, litteral[0], 7 };
		an_view->on_changed = [this](char *) {};
		nav.push(an_view);
	};
	button_setam_b.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, litteral[1], 7 };
		an_view->on_changed = [this](char *) {};
		nav.push(an_view);
	};
	button_setam_c.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, litteral[2], 7 };
		an_view->on_changed = [this](char *) {};
		nav.push(an_view);
	};
	button_setam_d.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, litteral[3], 7 };
		an_view->on_changed = [this](char *) {};
		nav.push(an_view);
	};
	button_setam_e.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, litteral[4], 7 };
		an_view->on_changed = [this](char *) {};
		nav.push(an_view);
	};
	
	button_lcrdebug.on_select = [this,&nav](Button&){
		make_frame();
		nav.push(new DebugLCRView { nav, lcrstring, checksum });
	};
	
	button_transmit.on_select = [this,&transmitter_model](Button&){		
		auto& message_map = context().message_map();
		
		make_frame();
			
		shared_memory.afsk_samples_per_bit = 228000/portapack::persistent_memory::afsk_bitrate();
		shared_memory.afsk_phase_inc_mark = portapack::persistent_memory::afsk_mark_freq()*(0x40000*256)/2280;
		shared_memory.afsk_phase_inc_space = portapack::persistent_memory::afsk_space_freq()*(0x40000*256)/2280;

		shared_memory.afsk_fmmod = portapack::persistent_memory::afsk_bw() * 8;

		memset(shared_memory.lcrdata, 0, 256);
		memcpy(shared_memory.lcrdata, lcrframe_f, 256);
		
		shared_memory.afsk_transmit_done = false;
		shared_memory.afsk_repeat = (portapack::persistent_memory::afsk_config() >> 8) & 0xFF;

		message_map.register_handler(Message::ID::TXDone,
			[this,&transmitter_model](Message* const p) {
				char str[8];
				const auto message = static_cast<const TXDoneMessage*>(p);
				if (message->n > 0) {
					text_status.set("       ");
					strcpy(str, to_string_dec_int(message->n).c_str());
					strcat(str, "/");
					strcat(str, to_string_dec_int((portapack::persistent_memory::afsk_config() >> 8) & 0xFF).c_str());
					text_status.set(str);
				} else {
					text_status.set("Done ! ");
					transmitter_model.disable();
				}
			}
		);

		char str[8];
		strcpy(str, "0/");
		strcat(str, to_string_dec_int(shared_memory.afsk_repeat).c_str());
		text_status.set(str);
		
		transmitter_model.enable();
	};
	
	button_txsetup.on_select = [&nav, &transmitter_model](Button&){
		nav.push(new AFSKSetupView { nav, transmitter_model });
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
