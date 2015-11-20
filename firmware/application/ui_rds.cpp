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

#include "ch.h"

#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace hackrf::one;

namespace ui {

void RDSView::focus() {
	button_setpsn.focus();
}

RDSView::~RDSView() {
	transmitter_model.disable();
}

std::string to_string_bin(const uint32_t n, const uint8_t l) {
	char p[32];
	for (uint8_t c = 0; c < l; c++) {
		if ((n<<c) & (1<<l))
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
        doinv = (((blockdata<<i) & 0x8000)>>15) ^ (CRC>>9);
        if (doinv) CRC ^= 0b0011011100;
        CRC = ((CRC<<1) | doinv) & 0x3FF;
    }
    
    return (blockdata<<10) | (CRC ^ offset);
}

//Make PI
//Set frequency
//TA/TP flags
//Group selection
//RST SNCF
//Jammer
//Microphone troll
//CTCSS

void RDSView::paint(Painter& painter) {
	uint8_t c;
	
	uint32_t group[4][4] = {
		{0b1111100001001001,		//PI
		0b0000110011101000,			//Address
		0b1111100001001001,			//PI
		0b0000000000000000},		//Replaced
		{0b1111100001001001,		//PI
		0b0000110011101001,			//Address
		0b1111100001001001,			//PI
		0b0000000000000000},		//Replaced
		{0b1111100001001001,		//PI
		0b0000110011101010,			//Address
		0b1111100001001001,			//PI
		0b0000000000000000},		//Replaced
		{0b1111100001001001,		//PI
		0b0000110011101011,			//Address
		0b1111100001001001,			//PI
		0b0000000000000000},		//Replaced
	};
	
	//Insert PSN data in groups
	group[0][3] = (psname[0]<<8) | psname[1];
	group[1][3] = (psname[2]<<8) | psname[3];
	group[2][3] = (psname[4]<<8) | psname[5];
	group[3][3] = (psname[6]<<8) | psname[7];

	//Generate checkbits
	for (c = 0; c < 4; c++) {
		group[c][0] = makeblock(group[c][0], 0b0011111100);
		group[c][1] = makeblock(group[c][1], 0b0110011000);
		group[c][2] = makeblock(group[c][2], 0b1101010000);	//C'
		group[c][3] = makeblock(group[c][3], 0b0110110100);
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
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	transmitter_model.set_tuning_frequency(93000000);
	strcpy(psname, "TEST1234");
	
	add_children({ {
		&text_title,
		&button_setpsn,
		&button_transmit,
		&button_exit
	} });
	
	button_setpsn.on_select = [this,&nav](Button&){
		auto an_view = new AlphanumView { nav, psname, 8 };
		nav.push(an_view);
	};
	
	button_transmit.on_select = [&transmitter_model](Button&){
		transmitter_model.enable();
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
