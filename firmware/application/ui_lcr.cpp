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
	button_setam_a.focus();
}

LCRView::~LCRView() {
	transmitter_model.disable();
}

char hexify(char in) {
	if (in > 9) in += 7;
	return in + 0x30;
}

void LCRView::paint(Painter& painter) {
	char eom[3] = { 3, 0, 0 };
	uint8_t checksum = 0, i;
	char teststr[16];
	uint16_t dp;
	uint8_t cp, pp, cur_byte, new_byte;
	
	static constexpr Style style_orange {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::orange(),
	};
	
	Point offset = {
		static_cast<Coord>(72),
		static_cast<Coord>(72)
	};
	
	painter.draw_string(
		screen_pos() + offset,
		style_orange,
		litteral[0]
	);
	
	offset.y += 40;
	
	painter.draw_string(
		screen_pos() + offset,
		style_orange,
		litteral[1]
	);
	
	offset.y += 40;
	
	painter.draw_string(
		screen_pos() + offset,
		style_orange,
		litteral[2]
	);
	
	offset.y += 40;
	
	painter.draw_string(
		screen_pos() + offset,
		style_orange,
		litteral[3]
	);
	
	// Testing: 7 char pad for litterals
	for (i = 0; i < 4; i++) {
		while (strlen(litteral[i]) < 7) {
			strcat(litteral[i], " ");
		}
	}
	
	// Recreate LCR frame
	memset(lcrframe, 0, 256);
	lcrframe[0] = 127;
	lcrframe[1] = 127;
	lcrframe[2] = 127;
	lcrframe[3] = 127;
	lcrframe[4] = 127;
	lcrframe[5] = 127;
	lcrframe[6] = 127;
	lcrframe[7] = 15;
	strcat(lcrframe, rgsb);
	strcat(lcrframe, "PA AM=1 AF=\"");
	strcat(lcrframe, litteral[0]);
	strcat(lcrframe, "\" CL=0 AM=2 AF=\"");
	strcat(lcrframe, litteral[1]);
	strcat(lcrframe, "\" CL=0 AM=3 AF=\"");
	strcat(lcrframe, litteral[2]);
	strcat(lcrframe, "\" CL=0 AM=4 AF=\"");
	strcat(lcrframe, litteral[3]);
	strcat(lcrframe, "\" CL=0 EC=A SAB=0");		//TODO: EC=A,J,N
	
	//Checksum
	i = 7;
	while (lcrframe[i]) {
		checksum ^= lcrframe[i];
		i++;
	}
	checksum ^= 3;
	checksum &= 0x7F;
	eom[1] = checksum;
	
	strcat(lcrframe, eom);
	
	for (dp=0;dp<strlen(lcrframe);dp++) {
		pp = 0;
		new_byte = 0;
		cur_byte = lcrframe[dp];
		for (cp=0;cp<7;cp++) {
			if ((cur_byte>>cp)&1) pp++;
			new_byte |= ((cur_byte>>cp)&1)<<(7-cp);
		}
		lcrframe[dp] = new_byte|(pp&1);
	}
	
	teststr[0] = hexify(eom[1] >> 4);
	teststr[1] = hexify(eom[1] & 15);
	teststr[2] = 0;
	offset.x = 220;
	painter.draw_string(
		screen_pos() + offset,
		style(),
		teststr
	);
}

void LCRView::updfreq(rf::Frequency f) {
	char finalstr[9] = {0};
	transmitter_model.set_tuning_frequency(f);
	
	auto mhz = to_string_dec_int(f / 1000000, 3);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	strcat(finalstr, mhz.c_str());
	strcat(finalstr, ".");
	strcat(finalstr, hz100.c_str());

	this->button_setfreq.set_text(finalstr);
}

LCRView::LCRView(
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	transmitter_model.set_modulation(16);
	transmitter_model.set_tuning_frequency(f);
	memset(litteral, 0, 4*8);
	memset(rgsb, 0, 5);
	
	strcpy(rgsb, RGSB_list[0]);
	
	add_children({ {
		&button_setrgsb,
		&button_txsetup,
		&button_setam_a,
		&button_setam_b,
		&button_setam_c,
		&button_setam_d,
		&button_setfreq,
		&button_setbps,
		&button_transmit,
		&button_transmit_scan,
		&button_exit
	} });
	
	button_setrgsb.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, rgsb, 4 };
		an_view->on_changed = [this](char *rgsb) {
			button_setrgsb.set_text(rgsb);
		};
		nav.push(an_view);
	};
	button_setfreq.on_select = [this,&nav](Button&){
		auto new_view = new FrequencyKeypadView { nav, this->transmitter_model.tuning_frequency() };
		new_view->on_changed = [this](rf::Frequency f) {
			updfreq(f);
		};
		nav.push(new_view);
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
	button_setbps.on_select = [this](Button&){
		if (persistent_memory::afsk_bitrate() == 1200) {
			persistent_memory::set_afsk_bitrate(2400);
			button_setbps.set_text("2400 bps");
		} else {
			persistent_memory::set_afsk_bitrate(1200);
			button_setbps.set_text("1200 bps");
		}
	};
	
	button_transmit.on_select = [this,&transmitter_model](Button&){
		uint16_t c;
			
		shared_memory.afsk_samples_per_bit = 228000/persistent_memory::afsk_bitrate();
		shared_memory.afsk_phase_inc_mark = persistent_memory::afsk_mark_freq()*(65536*1024)/228000;
		shared_memory.afsk_phase_inc_space = persistent_memory::afsk_space_freq()*(65536*1024)/228000;

		for (c = 0; c < 256; c++) {
			shared_memory.lcrdata[c] = this->lcrframe[c];
		}
		transmitter_model.enable();
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
