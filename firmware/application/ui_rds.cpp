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

#include "ch.h"

#include "ui_alphanum.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void RDSView::focus() {
	button_setpsn.focus();
}

RDSView::~RDSView() {
	transmitter_model.disable();
}

std::string to_string_bin(const uint32_t n, const uint8_t l) {
	char p[33];
	for (uint8_t c = 0; c < l; c++) {
		if ((n << c) & (1 << l))
			p[c] = '1';
		else
			p[c] = '0';
	}
	p[l] = 0;
	return p;
}

uint32_t makeblock(uint32_t blockdata, uint16_t offset) {
    uint16_t CRC = 0;
    uint8_t doinv;
    
    for (uint8_t i = 0; i < 16; i++) {
        doinv = (((blockdata << i) & 0x8000) >> 15) ^ (CRC >> 9);
        if (doinv) CRC ^= 0b0011011100;
        CRC = ((CRC << 1) | doinv) & 0x3FF;
    }
    
    return (blockdata << 10) | (CRC ^ offset);
}

// Todo:
// Make PI
// Set frequency
// TA/TP flags
// Group selection

uint8_t RDSView::b2b(const bool in) {
	if (in)
		return 1;
	else
		return 0;
}

void RDSView::make_0B_group(uint32_t group[], const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool TA,
							const bool MS, const bool DI, const uint8_t C, char * chars) {

	group[0] = PI_code;
	group[1] = (0x0 << 12) | (1 << 11) | (b2b(TP) << 10) | (PTY << 5) | (b2b(TA) << 4) | (b2b(MS) << 3) | (b2b(DI) << 2) | (C & 3);
	group[2] = PI_code;
	group[3] = (chars[0] << 8) | chars[1];
}

void RDSView::paint(Painter& painter) {
	uint8_t c;
	uint32_t group[4][4] = { 0 };

	make_0B_group(&group[0][0], 0xF849, true, 7, false, true, false, 0, &psname[0]);
	make_0B_group(&group[1][0], 0xF849, true, 7, false, true, false, 1, &psname[2]);
	make_0B_group(&group[2][0], 0xF849, true, 7, false, true, false, 2, &psname[4]);
	make_0B_group(&group[3][0], 0xF849, true, 7, false, true, false, 3, &psname[6]);
	
	/*uint32_t group[4][4] = {
		{
		0b1111100001001001,		//PI
		0b0000110011101000,		//Address
		0b1111100001001001,		//PI
		0b0000000000000000		//Replaced
		},
		
		{
		0b1111100001001001,		//PI
		0b0000110011101001,		//Address
		0b1111100001001001,		//PI
		0b0000000000000000		//Replaced
		},
		
		{
		0b1111100001001001,		//PI
		0b0000110011101010,		//Address
		0b1111100001001001,		//PI
		0b0000000000000000		//Replaced
		},
		
		{
		0b1111100001001001,		//PI
		0b0000110011101011,		//Address
		0b1111100001001001,		//PI
		0b0000000000000000		//Replaced
		},
	};
	
	//Insert PSN data in groups
	group[0][3] = (psname[0] << 8) | psname[1];
	group[1][3] = (psname[2] << 8) | psname[3];
	group[2][3] = (psname[4] << 8) | psname[5];
	group[3][3] = (psname[6] << 8) | psname[7];
	*/
	
	//Generate checkbits
	for (c = 0; c < 4; c++) {
		group[c][0] = makeblock(group[c][0], RDS_OFFSET_A);
		group[c][1] = makeblock(group[c][1], RDS_OFFSET_B);
		group[c][2] = makeblock(group[c][2], RDS_OFFSET_Cp);	//C'
		group[c][3] = makeblock(group[c][3], RDS_OFFSET_D);
	}

	const Point offset = {
		static_cast<Coord>(64),
		static_cast<Coord>(32)
	};

	const auto text = psname;
	painter.draw_string(
		screen_pos() + offset,
		style(),
		text
	);
	
	for (c = 0; c < 16; c++) {
		shared_memory.rdsdata[c] = group[c >> 2][c & 3];
	}
}

RDSView::RDSView(
	NavigationView& nav
)
{
	strcpy(psname, "TEST1234");
	
	add_children({ {
		&field_frequency,
		&options_pty,
		&button_setpsn,
		&button_transmit,
		&button_exit
	} });
	
	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(100000);
	field_frequency.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->field_frequency.set_value(f);
		};
	};
	
	options_pty.set_selected_index(0);
	
	button_setpsn.on_select = [this,&nav](Button&){
		auto an_view =  nav.push<AlphanumView>(psname, 8);
		an_view->on_changed = [this](char *value) {
			memcpy(psname, value, 9);
		};
	};
	
	button_transmit.on_select = [this](Button&){
		transmitter_model.enable();
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
