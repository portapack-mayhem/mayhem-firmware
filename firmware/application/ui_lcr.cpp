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

#include "ui_alphanum.hpp"
#include "ui_rds.hpp"
#include "ui_lcr.hpp"
#include "ui_receiver.hpp"
#include "ui_afsksetup.hpp"
#include "ui_debug.hpp"

#include "ch.h"

#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"
#include "radio.hpp"

#include "string_format.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

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
	
	button_setrgsb.set_text(rgsb);

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
		for (dp = 0; dp < strlen(lcrframe); dp++) {
			pp = pm;
			new_byte = 0;
			cur_byte = lcrframe[dp];
			for (cp = 0; cp < 7; cp++) {
				if ((cur_byte >> cp) & 1) pp++;
				new_byte |= (((cur_byte >> cp) & 1) << (7 - cp));
			}
			lcrframe_f[dp] = new_byte | (pp & 1);
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
		static_cast<Coord>(68)
	};
	
	for (i = 0; i < 5; i++) {
		painter.draw_string(
			screen_pos() + offset,
			style_orange,
			litteral[i]
		);
		offset.y += 32;
	}
}

void LCRView::start_tx() {
	char str[16];
	
	if (scanning) {
		scan_index = 0;
		strcpy(rgsb, RGSB_list[0]);
	}
	
	make_frame();
	
	transmitter_model.set_tuning_frequency(portapack::persistent_memory::tuned_frequency());
		
	shared_memory.afsk_samples_per_bit = 228000/portapack::persistent_memory::afsk_bitrate();
	shared_memory.afsk_phase_inc_mark = portapack::persistent_memory::afsk_mark_freq()*(0x40000*256)/2280;
	shared_memory.afsk_phase_inc_space = portapack::persistent_memory::afsk_space_freq()*(0x40000*256)/2280;

	shared_memory.afsk_fmmod = portapack::persistent_memory::afsk_bw() * 8;

	memset(shared_memory.lcrdata, 0, 256);
	memcpy(shared_memory.lcrdata, lcrframe_f, 256);
	
	shared_memory.afsk_transmit_done = false;
	shared_memory.afsk_repeat = 5; //(portapack::persistent_memory::afsk_config() >> 8) & 0xFF;
	
	EventDispatcher::message_map().unregister_handler(Message::ID::TXDone);

	EventDispatcher::message_map().register_handler(Message::ID::TXDone,
		[this](Message* const p) {
			char str[16];
			
			if (abort_scan) {
				text_status.set("            ");
				strcpy(str, "Abort @");
				strcat(str, rgsb);
				//strcat(str, to_string_dec_int((portapack::persistent_memory::afsk_config() >> 8) & 0xFF).c_str());
				text_status.set(str);
				progress.set_value(0);
				transmitter_model.disable();
				txing = false;
				scanning = false;
				abort_scan = false;
				button_scan.set_style(&style_val);
				button_scan.set_text("SCAN");
				return;
			}
			
			const auto message = static_cast<const TXDoneMessage*>(p);
			if (message->n > 0) {
				if (scanning) {
					scan_progress += 0.555f;
					progress.set_value(scan_progress);
				} else {
					text_status.set("            ");
					strcpy(str, to_string_dec_int(6 - message->n).c_str());
					strcat(str, "/5");
					//strcat(str, to_string_dec_int((portapack::persistent_memory::afsk_config() >> 8) & 0xFF).c_str());
					text_status.set(str);
					progress.set_value((6 - message->n) * 20);
				}
			} else {
				if (scanning && (scan_index < 36)) {
					transmitter_model.disable();
					
					// Next address
					strcpy(rgsb, RGSB_list[scan_index]);
					make_frame();
					
					memset(shared_memory.lcrdata, 0, 256);
					memcpy(shared_memory.lcrdata, lcrframe_f, 256);
					shared_memory.afsk_transmit_done = false;
					shared_memory.afsk_repeat = 5;
					
					text_status.set("            ");
					strcpy(str, to_string_dec_int(scan_index).c_str());
					strcat(str, "/36");
					text_status.set(str);
					scan_progress += 0.694f;
					progress.set_value(scan_progress);
					
					scan_index++;
					transmitter_model.enable();
				} else {
					text_status.set("Ready       ");
					progress.set_value(0);
					transmitter_model.disable();
					txing = false;
					scanning = false;
					button_scan.set_style(&style_val);
					button_scan.set_text("SCAN");
				}
			}
		}
	);

	if (scanning) {
		text_status.set("            ");
		strcat(str, "1/36");
		text_status.set(str);
		progress.set_value(1);
		scan_index++;
	} else {
		strcpy(str, "1/5         ");
		//strcat(str, to_string_dec_int(shared_memory.afsk_repeat).c_str());
		text_status.set(str);
		progress.set_value(20);
	}
	
	txing = true;
	transmitter_model.enable();
}

LCRView::LCRView(
	NavigationView& nav
)
{
	char finalstr[24] = {0};
	
	transmitter_model.set_baseband_configuration({
		.mode = 3,
		.sampling_rate = 2280000,	// Is this right ?
		.decimation_factor = 1,
	});

	memset(litteral, 0, 5 * 8);
	memset(rgsb, 0, 5);
	
	strcpy(rgsb, RGSB_list[0]);
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
		&progress,
		&button_lcrdebug,
		&button_transmit,
		&button_scan,
		&button_clear
	} });
	
	checkbox_am_a.set_value(true);
	checkbox_am_b.set_value(false);
	checkbox_am_c.set_value(false);
	checkbox_am_d.set_value(false);
	checkbox_am_e.set_value(false);
	
	// Recap: tx freq @ bps
	auto fstr = to_string_dec_int(portapack::persistent_memory::tuned_frequency() / 1000, 6);
	auto bstr = to_string_dec_int(portapack::persistent_memory::afsk_bitrate(), 4);
	strcat(finalstr, fstr.c_str());
	strcat(finalstr, " @ ");
	strcat(finalstr, bstr.c_str());
	strcat(finalstr, "bps");
	text_recap.set(finalstr);
	
	button_transmit.set_style(&style_val);
	button_scan.set_style(&style_val);
	
	button_setrgsb.on_select = [this,&nav](Button&) {
		auto an_view =  nav.push<AlphanumView>(rgsb, 4);
		an_view->on_changed = [this](char *rgsb) {
			button_setrgsb.set_text(rgsb);
		};
	};
	
	button_setam_a.on_select = [this,&nav](Button&) {
		auto an_view = nav.push<AlphanumView>(litteral[0], 7);
		an_view->on_changed = [this](char *) {};
	};
	button_setam_b.on_select = [this,&nav](Button&) {
		auto an_view = nav.push<AlphanumView>(litteral[1], 7);
		an_view->on_changed = [this](char *) {};
	};
	button_setam_c.on_select = [this,&nav](Button&) {
		auto an_view = nav.push<AlphanumView>(litteral[2], 7);
		an_view->on_changed = [this](char *) {};
	};
	button_setam_d.on_select = [this,&nav](Button&) {
		auto an_view = nav.push<AlphanumView>(litteral[3], 7);
		an_view->on_changed = [this](char *) {};
	};
	button_setam_e.on_select = [this,&nav](Button&) {
		auto an_view = nav.push<AlphanumView>(litteral[4], 7);
		an_view->on_changed = [this](char *) {};
	};
	
	button_txsetup.on_select = [&nav](Button&) {
		nav.push<AFSKSetupView>();
	};
	
	button_lcrdebug.on_select = [this,&nav](Button&) {
		make_frame();
		nav.push<DebugLCRView>(lcrstring, checksum);
	};
	
	button_transmit.on_select = [this](Button&) {
		if (txing == false)	start_tx();
	};
	
	button_scan.on_select = [this](Button&) {
		if (txing == false)	{
			scanning = true;
			scan_progress = 0;
			button_scan.set_style(&style_cancel);
			button_scan.set_text("ABORT");
			start_tx();
		} else {
			abort_scan = true;
		}
	};

	button_clear.on_select = [this, &nav](Button&) {
		memset(litteral, 0, 5 * 8);
		checkbox_am_a.set_value(true);
		checkbox_am_b.set_value(true);
		checkbox_am_c.set_value(true);
		checkbox_am_d.set_value(true);
		checkbox_am_e.set_value(true);
		set_dirty();
		start_tx();
	};
}

} /* namespace ui */
