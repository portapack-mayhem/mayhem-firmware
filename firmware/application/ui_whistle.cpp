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

#include "ui_whistle.hpp"
#include "ui_receiver.hpp"

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

void WhistleView::focus() {
	button_transmit.focus();
}

WhistleView::~WhistleView() {
	transmitter_model.disable();
}

void WhistleView::paint(Painter& painter) {

}

void WhistleView::whistle_th(void *arg) {
	Mailbox* mbox = (Mailbox *)arg;
	chMBPost(mbox, 1, TIME_INFINITE);
}

Button WhistleView::button_scan = {
	{ 76, 270, 72, 32 },
	"SCAN"
};

WhistleView::WhistleView(
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	Mailbox mbox;
    msg_t mbox_buffer[3];
    chMBInit(&mbox, mbox_buffer, 3);
	
	transmitter_model.set_modulation(17);
	transmitter_model.set_tuning_frequency(persistent_memory::tuned_frequency());
	
	add_children({ {
		&button_scan,
		&button_transmit,
		&button_exit
	} });
	
	button_transmit.on_select = [this,&transmitter_model](Button&){
		/*uint16_t c;
		ui::Context context;
		
		make_frame();
			
		shared_memory.afsk_samples_per_bit = 228000/persistent_memory::afsk_bitrate();
		shared_memory.afsk_phase_inc_mark = persistent_memory::afsk_mark_freq()*(65536*1024)/2280;
		shared_memory.afsk_phase_inc_space = persistent_memory::afsk_space_freq()*(65536*1024)/2280;

		for (c = 0; c < 256; c++) {
			shared_memory.lcrdata[c] = this->lcrframe[c];
		}
		
		shared_memory.afsk_transmit_done = false;
		shared_memory.afsk_repeat = 5;		// DEFAULT

		text_status.set("Send...");
		
		transmitter_model.enable();*/
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};

	static VirtualTimer vt1;
	msg_t msg, result;

	/*static EvTimer evt;
	static EventListener el0;
	
	evtInit(&evt, MS2ST(2000));
	evtStart(&evt);
	chEvtRegister(&evt.et_es, &el0, 0);*/
	
	//chVTSet(&vt1, MS2ST(2000), whistle_th, (void *)&mbox);
	
    while(1) {
        /*result = chMBFetch(&mbox, &msg, TIME_INFINITE);
        if(result == RDY_OK) {
            if(msg & 1)
                button_scan.set_text("POUET");
        }*/
        chThdSleepMilliseconds(500);
    }
	
}

} /* namespace ui */
