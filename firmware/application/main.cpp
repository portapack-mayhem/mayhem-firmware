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

//TEST: Menuview refresh, seems to blink a lot
//TEST: Check AFSK transmit end, skips last bits ?
//TEST: Imperial in whipcalc

//BUG: Auto backlight off doesn't work anymore
//BUG: CPLD-related rx ok, tx bad, see portapack.cpp lines 214+ to disable CPLD overlay
//BUG: REPLAY See what's wrong with quality (format, or need for interpolation filter ?)
//BUG: SCANNER Lock on frequency, if frequency jump, still locked on first one
//BUG: SCANNER Multiple slices

//TODO: Cap Wav viewer position
//TODO: Adapt wav viewer position step
//TODO: Use unit_auto_scale
//TODO: Remove make_bistream from encoders.cpp, too complex, stinks. bitstream_append should be enough.
//TODO: Continue work on proc_afskrx_corr, see python script (it works !)
//TODO: Super simple text file viewer
//TODO: De bruijn sequence scanner for encoders
//TODO: FILEMAN Rename folders
//TODO: FILEMAN Move files
//TODO: Frequency and bw settings were removed from modemsetup, put those back in LCR TX
//TODO: Use separate thread for scanning in EPAR TX
//TODO: Use separate thread for scanning in LCR TX
//TODO: REPLAY Convert C16 to C8 on M0 core
//TODO: Make freqman refresh simpler (use previous black rectangle method)
//TODO: Merge AFSK and TONES procs ?
//TODO: NFM RX mode: nav.pop on squelch
//TODO: MORSE use prosigns
//TODO: MORSE live keying mode
//TODO: Use to_string_short_freq wherever possible
//TODO: SCANNER Waveform widget as FFT view ?
//TODO: Optimize (and group ?) CTCSS tone gen code
/*
Continuous (Fox-oring)
12s transmit, 48s space (Sprint 1/5th) 
60s transmit, 240s space (Classic 1/5 min) 
60s transmit, 360s space (Classic 1/7 min) 
*/
//TODO: FreqMan: Remove and rename categories
//TODO: Mousejack ?
//TODO: Move frequencykeypad from ui_receiver to ui_widget (used everywhere)
//TODO: ADS-B draw trajectory + GPS coordinates + scale, and playback
//TODO: RDS multiple groups (sequence)
//TODO: Use ModalMessageView confirmation for TX ?
//TODO: Use msgpack for settings, lists... on sd card

// Multimon-style stuff:
//TODO: CTCSS detector
//TODO: DMR detector
//TODO: GSM channel detector
//TODO: SIGFOX RX/TX
//TODO: Playdead amnesia and login
//TODO: Setup: Play dead by default ? Enable/disable ?

// Old or low-priority stuff:
//TODO: Bodet :)
//TODO: Analog TV tx with camcorder font character generator
//TODO: Show address/data bit fields in OOK TX
//TODO: Scan for OOK TX
//TODO: Script engine ?
//TODO: AFSK receiver
//TODO: Check more OOK encoders
//BUG (fixed ?): No audio in about when shown second time
//TODO: Show MD5 mismatches for modules not found, etc...
//TODO: Module name/filename in modules.hpp to indicate requirement in case it's not found ui_loadmodule
//BUG: Description doesn't show up first time going to system>module info (UI drawn on top)
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
	if( portapack::init() ) {
		portapack::display.init();

		sdcStart(&SDCD1, nullptr);

		controls_init();
		lcd_frame_sync_configure();
		rtc_interrupt_enable();

		event_loop();

		sdcDisconnect(&SDCD1);
		sdcStop(&SDCD1);

		portapack::shutdown();
	}

	m4_init(portapack::spi_flash::image_tag_hackrf, portapack::memory::map::m4_code_hackrf);
	m0_halt();

	return 0;
}
