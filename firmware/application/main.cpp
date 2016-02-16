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

#include "core_control.hpp"
#include "spi_image.hpp"

#include "debug.hpp"
#include "led.hpp"

#include "gcc.hpp"

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
	m0_halt();

	return 0;
}
