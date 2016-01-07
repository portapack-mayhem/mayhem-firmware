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

#include "m4_startup.hpp"

#include "hal.h"
#include "lpc43xx_cpp.hpp"
#include "message.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

char * modhash;

/* TODO: OK, this is cool, but how do I put the M4 to sleep so I can switch to
 * a different image? Other than asking the old image to sleep while the M0
 * makes changes?
 *
 * I suppose I could force M4MEMMAP to an invalid memory reason which would
 * cause an exception and effectively halt the M4. But that feels gross.
 */
void m4_init(const portapack::spi_flash::region_t from, const portapack::memory::region_t to) {
	/* Initialize M4 code RAM */
	// DEBUG
	std::memcpy(reinterpret_cast<void*>(to.base()), from.base(), from.size);

	/* M4 core is assumed to be sleeping with interrupts off, so we can mess
	 * with its address space and RAM without concern.
	 */
	LPC_CREG->M4MEMMAP = to.base();

	/* Reset M4 core */
	LPC_RGU->RESET_CTRL[0] = (1 << 13);
}

int m4_load_image(void) {
	const char magic[6] = {'P', 'P', 'M', ' ', 0x01, 0x00};
	//uint32_t mod_size;
	UINT bw;
	uint8_t i;
	uint16_t cnt;
	char md5sum[16];
	FILINFO modinfo;
	FIL modfile;
	DIR rootdir;
	FRESULT res;
	
	// Scan SD card root directory for files with the right MD5 fingerprint at the right location
	f_opendir(&rootdir, "/");
	for (;;) {
		res = f_readdir(&rootdir, &modinfo);
		if (res != FR_OK || modinfo.fname[0] == 0) break;	// Reached last file, abort
		// Only care about files with .bin extension
		if ((!(modinfo.fattrib & AM_DIR)) && (modinfo.fname[9] == 'B') && (modinfo.fname[10] == 'I') && (modinfo.fname[11] == 'N')) {
			f_open(&modfile, modinfo.fname, FA_OPEN_EXISTING | FA_READ);
			// Magic bytes and version check
			f_read(&modfile, &md5sum, 6, &bw);
			for (i = 0; i < 6; i++) {
				if (md5sum[i] != magic[i]) break;
			}
			if (i == 6) {
				f_lseek(&modfile, 26);
				f_read(&modfile, &md5sum, 16, &bw);
				for (i = 0; i < 16; i++) {
					if (md5sum[i] != modhash[i]) break;
				}
				if (i == 16) {
					//f_lseek(&modfile, 6);
					//f_read(&modfile, &mod_size, 4, &bw);
					f_lseek(&modfile, 256);
					// For some reason, f_read > 512 bytes at once crashes everything... :/
					for (cnt=0;cnt<256;cnt++)
						f_read(&modfile, reinterpret_cast<void*>(portapack::memory::map::m4_code.base()+(cnt*256)), 256, &bw);
					f_close(&modfile);
					LPC_RGU->RESET_CTRL[0] = (1 << 13);
					return 1;
				}
			}
			f_close(&modfile);
		}
	}
	
	return 0;
}

void m4_switch(const char * hash) {
	modhash = const_cast<char*>(hash);
	
	// Ask M4 to enter wait loop in RAM
	BasebandConfiguration baseband_switch {
		.mode = SWITCH,
		.sampling_rate = 0,
		.decimation_factor = 1,
	};
	
	BasebandConfigurationMessage message { baseband_switch };
	shared_memory.baseband_queue.push(message);
}

void m4_request_shutdown() {
	ShutdownMessage shutdown_message;
	shared_memory.baseband_queue.push(shutdown_message);
}
