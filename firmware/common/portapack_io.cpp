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

#include "portapack_io.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "utility.hpp"

#include <ch.h>

#include <cstdint>

namespace portapack {

void IO::init() {
	data_mask_set();
	data_write_high(0);

	dir_read();
	lcd_rd_deassert();
	lcd_wr_deassert();
	io_stb_deassert();
	addr(0);

	gpio_dir.output();
	gpio_lcd_rdx.output();
	gpio_lcd_wrx.output();
	gpio_io_stbx.output();
	gpio_addr.output();
	gpio_rot_a.input();
	gpio_rot_b.input();
}

void IO::lcd_backlight(const bool value) {
	io_reg = (io_reg & 0x7f) | ((value ? 1 : 0) << 7);
	io_write(1, io_reg);
}

void IO::lcd_reset_state(const bool active) {
	io_reg = (io_reg & 0xfe) | ((active ? 1 : 0) << 0);
	io_write(1, io_reg);
}

void IO::audio_reset_state(const bool active) {
	/* NOTE: This overwrites the contents of the IO register, which for now
	 * have no significance. But someday...?
	 */
	io_reg = (io_reg & 0xfd) | ((active ? 1 : 0) << 1);
	io_write(1, io_reg);
}

void IO::reference_oscillator(const bool enable) {
	const uint8_t mask = 1 << 6;
	io_reg = (io_reg & ~mask) | (enable ? mask : 0);
	io_write(1, io_reg);
}

uint32_t IO::io_update(const TouchPinsConfig write_value) {
	/* Very touchy code to save context of PortaPack data bus while the
	 * resistive touch pin drive is changed. Order of operations is
	 * important to prevent latching spurious data into the LCD or IO
	 * registers.
	 */

	const auto save_data = data_read();
	const auto addr = gpio_addr.read();
	const auto dir = gpio_dir.read();

	io_stb_assert();

	/* Switch to read */
	dir_read();
	addr_0();
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	const auto switches_raw = data_read();

	/* Switch to write */
	data_write_low(toUType(write_value));
	dir_write();
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	io_stb_deassert();

	data_write_low(save_data);
	if( dir ) {	/* 0 (write) -> 1 (read) */
		dir_read();
	}
	gpio_addr.write(addr);

	return switches_raw;
}

}
