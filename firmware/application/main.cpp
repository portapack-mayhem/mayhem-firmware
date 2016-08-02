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

// Bitmaps generated with:
// Gimp image > indexed colors (16), then "xxd -i *.bmp"

//BUG: No audio in about when shown second time

//TODO: Check AFSK transmit end, skips last bits ?
//TODO: Use msgpack for settings, lists... on sd card
//TODO: Frequency manager
//TODO: Morse coder

//Multimon-style stuff:
//TODO: AFSK receiver
//TODO: Xylos receiver

//TODO: Check jammer bandwidths
//TODO: Closecall wide range fix
//TODO: SD card wiper
//TODO: Use progressbars
//TODO: GSM channel detector
//TODO: SIGFOX RX/TX
//TODO: Bodet :)
//TODO: Whistler

//TODO: LCR full message former (see norm)
//TODO: AFSK NRZI, parity and format
//TODO: TX power setting

//TODO: Playdead amnesia and login
//TODO: Setup: Play dead by default ? Enable/disable ?
//TODO: Hide statusview when playing dead
//TODO: Persistent playdead !
//TODO: Show MD5 mismatches for modules not found, etc...
//TODO: Module name/filename in modules.hpp to indicate requirement in case it's not found ui_loadmodule
//BUG: Description doesn't show up first time going to system>module info (UI drawn on top)

//TODO: Draw on touchscreen and transmit as spectrum paint
//TODO: Two players tic-tac-toe

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
