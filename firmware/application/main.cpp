/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

//BUG:  No audio in about when shown second time
//BUG:  Description doesn't show up first time going to system>module info (UI drawn on top)
//TODO: Morse coder
//TODO: Playdead amnesia and login
//TODO: Touch screen calibration
//TODO: Display module info (name, desc) somewhere
//TODO: Show MD5 mismatches for modules not found, etc...
//TODO: More gfx, cute icons :)
//TODO: Check jammer bandwidths
//TODO: GSM channel detector
//TODO: AFSK receiver
//TODO: SIGFOX RX/TX
//TODO: Reset baseband if module not found (instead of lockup in RAM loop)
//TODO: Module name/filename in modules.hpp to indicate requirement in case it's not found ui_loadmodule
//TODO: LCD backlight PWM
//TODO: BUG: Crash after TX stop (unregister message !)
//TODO: Check bw setting in LCR TX
//TODO: BUG: Crash after PSN entry in RDS TX
//TODO: Bodet :)
//TODO: Whistler
//TODO: Setup: Play dead by default ? Enable/disable ?
//TODO: Hide statusview when playing dead
//TODO: Persistent playdead !
//TODO: LCR EC=A,J,N
//TODO: LCR full message former (see norm)
//TODO: LCR address scan
//TODO: AFSK NRZI
//TODO: TX power

#include "ch.h"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "cpld_update.hpp"

#include "message_queue.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"

#include "irq_lcd_frame.hpp"
#include "irq_controls.hpp"
#include "irq_rtc.hpp"

#include "event_m0.hpp"

#include "m4_startup.hpp"
#include "spi_image.hpp"

#include "debug.hpp"
#include "led.hpp"

#include "gcc.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "sd_card.hpp"

#include <string.h>

int main(void) {
	portapack::init();

	if( !cpld_update_if_necessary() ) {
		chSysHalt();
	}

	portapack::io.init();
	portapack::display.init();

	sdcStart(&SDCD1, nullptr);

	init_message_queues();

	ui::Context context;
	ui::SystemView system_view {
		context,
		portapack::display.screen_rect()
	};
	ui::Painter painter;
	EventDispatcher event_dispatcher { &system_view, painter, context };

	EventDispatcher::message_map().register_handler(Message::ID::Shutdown,
		[&event_dispatcher](const Message* const) {
			event_dispatcher.request_stop();
		}
	);
	EventDispatcher::message_map().register_handler(Message::ID::DisplaySleep,
		[&event_dispatcher](const Message* const) {
			event_dispatcher.set_display_sleep(true);
		}
	);

	m4_init(portapack::spi_flash::baseband, portapack::memory::map::m4_code);

	controls_init();
	lcd_frame_sync_configure();
	rtc_interrupt_enable();

	event_dispatcher.run();

	sdcDisconnect(&SDCD1);
	sdcStop(&SDCD1);

	portapack::shutdown();
	m4_init(portapack::spi_flash::hackrf, portapack::memory::map::m4_code_hackrf);

	rgu::reset(rgu::Reset::M0APP);

	return 0;
}
