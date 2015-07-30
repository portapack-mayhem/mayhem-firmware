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

#include <cstdint>
#include <cstddef>
#include <cstring>

static constexpr uint32_t m4_text_flash_image_offset = 0x20000;
static constexpr size_t m4_text_size = 0x8000;
static constexpr uint32_t m4_text_flash_base = LPC_SPIFI_DATA_CACHED_BASE + m4_text_flash_image_offset;
static constexpr uint32_t m4_text_ram_base = 0x10080000;

/* TODO: OK, this is cool, but how do I put the M4 to sleep so I can switch to
 * a different image? Other than asking the old image to sleep while the M0
 * makes changes?
 *
 * I suppose I could force M4MEMMAP to an invalid memory reason which would
 * cause an exception and effectively halt the M4. But that feels gross.
 */
void m4_init() {
	/* Initialize M4 code RAM */
	std::memcpy((void*)m4_text_ram_base, (void*)m4_text_flash_base, m4_text_size);

	/* M4 core is assumed to be sleeping with interrupts off, so we can mess
	 * with its address space and RAM without concern.
	 */
	LPC_CREG->M4MEMMAP = m4_text_ram_base;

	/* Reset M4 core */
	LPC_RGU->RESET_CTRL[0] = (1 << 13);
}
