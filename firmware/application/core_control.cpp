/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
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

#include "core_control.hpp"

#include "hal.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "message.hpp"
#include "baseband_api.hpp"
#include "lz4.h"

#include <cstring>

void m4_init(const portapack::spi_flash::image_tag_t image_tag, const portapack::memory::region_t to, const bool full_reset) {
	const portapack::spi_flash::chunk_t* chunk = reinterpret_cast<const portapack::spi_flash::chunk_t*>(portapack::spi_flash::images.base());
	while(chunk->tag) {
		if(chunk->tag == image_tag) {

			const void *src = &chunk->data[0];
			void *dst = reinterpret_cast<void*>(to.base());

			/* extract and initialize M4 code RAM */
			unlz4_len(src, dst, chunk->compressed_data_size);

			/* M4 core is assumed to be sleeping with interrupts off, so we can mess
			 * with its address space and RAM without concern.
			 */
			LPC_CREG->M4MEMMAP = to.base();

			/* Reset M4 core and optionally all peripherals */
			LPC_RGU->RESET_CTRL[0] = (full_reset) ?
				  (1 << 1)  // PERIPH_RST
				: (1 << 13) // M4_RST
				;

			return;
		}
		chunk = chunk->next();
	}

	chDbgPanic("NoImg");
}

void m4_request_shutdown() {
	baseband::shutdown();
}

void m0_halt() {
	rgu::reset(rgu::Reset::M0APP);
	while(true) {
		port_wait_for_interrupt();
	}
}
