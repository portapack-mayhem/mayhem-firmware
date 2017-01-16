/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

// Color bitmaps generated with:
// Gimp image > indexed colors (16), then "xxd -i *.bmp"

//BUG: Bad console scroll init

//TEST: Imperial in whipcalc
//TEST: Numbers
//TEST: Jammer

//TODO: Frequency manager auto-remove duplicates
//TODO: "TX box" view or composite widget with frequency and bw settings, simple and advanced setup TX buttons...
//TODO: Morse coder for foxhunts
//TODO: Finish EPAR tx
//TODO: IQ replay
//TODO: Wav visualizer

//BUG: POCSAG RX sometimes misses the first codeword after SYNC
//BUG: Check AFSK transmit end, skips last bits ?
//BUG: RDS doesn't stop baseband when stopping tx ?

//TODO: File browser ?
//TODO: Mousejack ?
//TODO: Move frequencykeypad from ui_receiver to ui_widget (used everywhere)
//TODO: ADS-B draw trajectory + GPS coordinates + scale, and playback
//TODO: Analog TV tx with camcorder font character generator
//TODO: Make Morse coder and Whistle use proc_tones
//TODO: RDS multiple groups (sequence)
//TODO: Use ModalMessageView confirmation for TX ?
//TODO: Show address/data bit fields in OOK TX
//TODO: Scan for OOK TX
//TODO: Check more OOK encoders
//TODO: POCSAG 512 and 2400 (all 3 at the same time, or parameter ?)
//TODO: Use msgpack for settings, lists... on sd card

//Multimon-style stuff:
//TODO: AFSK receiver
//TODO: CTCSS detector
//TODO: DMR detector

//TODO: GSM channel detector
//TODO: SIGFOX RX/TX
//TODO: Bodet :)

//TODO: LCR full message former (see norm)
//TODO: AFSK NRZI
//TODO: TX power setting

//TODO: Playdead amnesia and login
//TODO: Setup: Play dead by default ? Enable/disable ?

//BUG (fixed ?): No audio in about when shown second time

//TODO: Show MD5 mismatches for modules not found, etc...
//TODO: Module name/filename in modules.hpp to indicate requirement in case it's not found ui_loadmodule
//BUG: Description doesn't show up first time going to system>module info (UI drawn on top)
//TODO: Draw on touchscreen and transmit as spectrum paint
//TODO: Two players tic-tac-toe
//TODO: Analog TV pong game

#include "ch.h"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "message_queue.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"

#include "irq_lcd_frame.hpp"
#include "irq_controls.hpp"
#include "irq_rtc.hpp"

#include "event_m0.hpp"

#include "core_control.hpp"
#include "spi_image.hpp"

#include "debug.hpp"
#include "led.hpp"

#include "gcc.hpp"

#include "sd_card.hpp"

#include <string.h>

static void event_loop() {
	ui::Context context;
	ui::SystemView system_view {
		context,
		portapack::display.screen_rect()
	};

	EventDispatcher event_dispatcher { &system_view, context };
	MessageHandlerRegistration message_handler_display_sleep {
		Message::ID::DisplaySleep,
		[&event_dispatcher](const Message* const) {
			event_dispatcher.set_display_sleep(true);
		}
	};

	event_dispatcher.run();
}

int main(void) {
	portapack::init();

	portapack::io.init();
	portapack::display.init();

	sdcStart(&SDCD1, nullptr);

	controls_init();
	lcd_frame_sync_configure();
	rtc_interrupt_enable();

	event_loop();

	sdcDisconnect(&SDCD1);
	sdcStop(&SDCD1);

	portapack::shutdown();
	m4_init(portapack::spi_flash::image_tag_hackrf, portapack::memory::map::m4_code_hackrf);
	m0_halt();

	return 0;
}
