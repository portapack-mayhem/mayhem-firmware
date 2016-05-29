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
	button_editpsn.focus();
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
							const bool MS, const bool DI, const uint8_t C, const char * chars) {

	group[0] = PI_code;
	group[1] = (0x0 << 12) | (1 << 11) | (b2b(TP) << 10) | (PTY << 5) | (b2b(TA) << 4) | (b2b(MS) << 3) | (b2b(DI) << 2) | (C & 3);
	group[2] = PI_code;
	group[3] = (chars[0] << 8) | chars[1];
}

void RDSView::make_2A_group(uint32_t group[], const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool AB,
							const bool segment, const char * chars) {

	group[0] = PI_code;
	group[1] = (0x0 << 12) | (1 << 11) | (b2b(TP) << 10) | (PTY << 5) | (b2b(AB) << 4) | (segment & 15);
	group[2] = (chars[0] << 8) | chars[1];
	group[3] = (chars[2] << 8) | chars[3];
}

void RDSView::gen_PSN(const char * psname) {
	uint8_t c;
	uint32_t group[4][4] = { 0 };

	make_0B_group(&group[0][0], 0xF849, true, options_pty.selected_index(), false, true, false, 0, &psname[0]);
	make_0B_group(&group[1][0], 0xF849, true, options_pty.selected_index(), false, true, false, 1, &psname[2]);
	make_0B_group(&group[2][0], 0xF849, true, options_pty.selected_index(), false, true, false, 2, &psname[4]);
	make_0B_group(&group[3][0], 0xF849, true, options_pty.selected_index(), false, true, false, 3, &psname[6]);
	
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
	
	// Generate checkbits
	for (c = 0; c < 4; c++) {
		group[c][0] = makeblock(group[c][0], RDS_OFFSET_A);
		group[c][1] = makeblock(group[c][1], RDS_OFFSET_B);
		group[c][2] = makeblock(group[c][2], RDS_OFFSET_Cp);	// C' !
		group[c][3] = makeblock(group[c][3], RDS_OFFSET_D);
	}
	
	for (c = 0; c < 16; c++)
		shared_memory.radio_data[c] = group[c >> 2][c & 3];
		
	shared_memory.bit_length = 4 * 4 * 26;
}

void RDSView::gen_RadioText(const char * radiotext) {
	size_t c, i;
	uint32_t * group;
	char radiotext_buffer[65] = { 0 };
	uint8_t rtlen, groups;
	
	strcpy(radiotext_buffer, radiotext);
	
	rtlen = strlen(radiotext_buffer);
	
	radiotext_buffer[rtlen] = 0x0D;
	
	// Pad to multiple of 4
	while(rtlen & 3) {
		radiotext_buffer[rtlen] = ' ';
		rtlen++;
	}

	groups = rtlen >> 2;	// 4 characters per group

	group = (uint32_t*)chHeapAlloc(0x0, 4 * groups * sizeof(uint32_t));

	for (c = 0; c < groups; c++)
		make_2A_group(&group[c << 2], 0xF849, true, options_pty.selected_index(), false, c, &radiotext_buffer[c << 2]);

	// Generate checkbits
	for (c = 0; c < groups; c++) {
		i = c * 4;
		group[i + 0] = makeblock(group[i + 0], RDS_OFFSET_A);
		group[i + 1] = makeblock(group[i + 1], RDS_OFFSET_B);
		group[i + 2] = makeblock(group[i + 2], RDS_OFFSET_C);
		group[i + 3] = makeblock(group[i + 3], RDS_OFFSET_D);
	}
	
	for (c = 0; c < (groups * 4); c++)
		shared_memory.radio_data[c] = group[c];
	
	shared_memory.bit_length = groups * 4 * 26;
}

void RDSView::paint(Painter& painter) {
	char RadioTextA[17];
	
	(void)painter;
	
	text_psn.set(PSN);
	memcpy(RadioTextA, RadioText, 16);
	RadioTextA[16] = 0;
	text_radiotexta.set(RadioTextA);
	text_radiotextb.set(&RadioText[16]);
}

RDSView::RDSView(
	NavigationView& nav
)
{
	transmitter_model.set_baseband_configuration({
		.mode = 5,
		.sampling_rate = 2280000,
		.decimation_factor = 1,
	});
	
	strcpy(PSN, "TEST1234");
	strcpy(RadioText, "Radiotext test !");
	
	add_children({ {
		&field_frequency,
		&options_pty,
		&options_countrycode,
		&options_coverage,
		&text_tx,
		&button_editpsn,
		&text_psn,
		&button_txpsn,
		&button_editradiotext,
		&text_radiotexta,
		&text_radiotextb,
		&button_txradiotext,
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
	
	options_pty.set_selected_index(0);				// None
	options_countrycode.set_selected_index(18);		// France
	options_coverage.set_selected_index(0);			// Local
	
	button_editpsn.on_select = [this,&nav](Button&){
		textentry(nav, PSN, 8);
	};
	button_txpsn.on_select = [this](Button&){
		if (txing) {
			transmitter_model.disable();
			button_txpsn.set_text("PSN");
			button_txradiotext.set_text("Radiotext");
			txing = false;
		} else {
			gen_PSN(PSN);
			transmitter_model.set_tuning_frequency(field_frequency.value());
			button_txpsn.set_text("STOP");
			txing = true;
			transmitter_model.enable();
		}
	};
	
	button_editradiotext.on_select = [this,&nav](Button&){
		textentry(nav, RadioText, 24);
	};
	button_txradiotext.on_select = [this](Button&){
		if (txing) {
			transmitter_model.disable();
			button_txpsn.set_text("PSN");
			button_txradiotext.set_text("Radiotext");
			txing = false;
		} else {
			gen_RadioText(RadioText);
			transmitter_model.set_tuning_frequency(field_frequency.value());
			button_txradiotext.set_text("STOP");
			txing = true;
			transmitter_model.enable();
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
