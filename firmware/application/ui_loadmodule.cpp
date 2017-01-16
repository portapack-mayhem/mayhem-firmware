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

#include "ui_loadmodule.hpp"

#include "ch.h"

#include "ff.h"
#include "event_m0.hpp"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
#include "hackrf_hal.hpp"
#include "string_format.hpp"

#include "ui_rds.hpp"
#include "ui_xylos.hpp"
#include "ui_epar.hpp"
#include "ui_lcr.hpp"
#include "analog_audio_app.hpp"
#include "ui_soundboard.hpp"
#include "ui_debug.hpp"
#include "ui_closecall.hpp"
#include "ui_audiotx.hpp"
#include "ui_jammer.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void LoadModuleView::focus() {
	button_ok.focus();
}

void LoadModuleView::on_show() {
	char md5_signature[16];
	uint8_t c;
	
	memcpy(md5_signature, (const void *)(0x10087FF0), 16);
	for (c=0; c<16; c++) {
		if (md5_signature[c] != _hash[c]) break;
	}
	//text_info.set(to_string_hex(*((unsigned int*)0x10087FF0), 8));
	
	if (c == 16) {
		text_info.set("Module already loaded :)");
		_mod_loaded = true;
	} else {
		text_info.set("Loading module");
		loadmodule();
	}
}

int LoadModuleView::load_image() {
	const char magic[6] = {'P', 'P', 'M', ' ', 0x02, 0x00};
	UINT bw;
	uint8_t i;
	uint32_t cnt;
	char md5sum[16];
	FILINFO modinfo;
	FIL modfile;
	DIR rootdir;
	FRESULT res;
	
	// Scan SD card root directory for files with the right MD5 fingerprint at the right location
	if (f_opendir(&rootdir, "/") == FR_OK) {
		for (;;) {
			res = f_readdir(&rootdir, &modinfo);
			if (res != FR_OK || modinfo.fname[0] == 0) break;	// Reached last file, abort
			// Only care about files with .bin extension
			if ((!(modinfo.fattrib & AM_DIR)) && (modinfo.fname[9] == 'B') && (modinfo.fname[10] == 'I') && (modinfo.fname[11] == 'N')) {
				res = f_open(&modfile, modinfo.fname, FA_OPEN_EXISTING | FA_READ);
				if (res != FR_OK) return 0;
				// Magic bytes and version check
				f_read(&modfile, &md5sum, 6, &bw);
				for (i = 0; i < 6; i++)
					if (md5sum[i] != magic[i]) break;
				if (i == 6) {
					f_lseek(&modfile, 26);
					f_read(&modfile, &md5sum, 16, &bw);
					for (i = 0; i < 16; i++)
						if (md5sum[i] != _hash[i]) break;
					// f_read can't read more than 512 bytes at a time ?
					if (i == 16) {
						f_lseek(&modfile, 512);
						for (cnt = 0; cnt < 64; cnt++) {
							if (f_read(&modfile, reinterpret_cast<void*>(portapack::memory::map::m4_code.base() + (cnt * 512)), 512, &bw)) return 0;
						}
						f_close(&modfile);
						f_closedir(&rootdir);
						LPC_RGU->RESET_CTRL[0] = (1 << 13);
						return 1;
					}
				}
				f_close(&modfile);
			}
		}
		f_closedir(&rootdir);
	}
	
	return 0;
}

void LoadModuleView::loadmodule() {
	//baseband::shutdown();
	
	/*EventDispatcher::message_map().register_handler(Message::ID::ReadyForSwitch,
		[this](Message* const p) {
			(void)p;*/
			if (load_image()) {
				text_infob.set("Module loaded :)");
				_mod_loaded = true;
			} else {
				text_info.set("Module not found :(");
				_mod_loaded = false;
			}
	//	}
	//);
	
}

LoadModuleView::LoadModuleView(
	NavigationView& nav,
	const char * hash,
	ViewID viewid
)
{

	add_children({
		&text_info,
		&text_infob,
		&button_ok
	});
	
	_hash = hash;

	button_ok.on_select = [this, &nav, viewid](Button&){
		nav.pop();
		if (_mod_loaded == true) {
			if (viewid == AudioTX) nav.push<AudioTXView>();
			if (viewid == Xylos) nav.push<XylosView>();
			if (viewid == EPAR) nav.push<EPARView>();
			if (viewid == LCR) nav.push<LCRView>();
			if (viewid == SoundBoard) nav.push<SoundBoardView>();
			if (viewid == AnalogAudio) nav.push<AnalogAudioView>();
			if (viewid == RDS) nav.push<RDSView>();
			if (viewid == CloseCall) nav.push<CloseCallView>();
			if (viewid == Receiver) nav.push<ReceiverMenuView>();
			if (viewid == Jammer) nav.push<JammerView>();
		}
	};
}

} /* namespace ui */
