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

#ifndef __PORTAPACK_HAL_H__
#define __PORTAPACK_HAL_H__

#include <cstdint>
#include <cstddef>
#include <array>

#include "pins.hpp"
using namespace lpc43xx;

namespace portapack {

/* TODO: Make these GPIOs private and expose via appropriate functions. */

constexpr GPIO gpio_io_stbx		= gpio[GPIO5_0];	/* P2_0 */
constexpr GPIO gpio_addr		= gpio[GPIO5_1];	/* P2_1 */
constexpr GPIO gpio_lcd_te		= gpio[GPIO5_3];	/* P2_3 */
constexpr GPIO gpio_unused		= gpio[GPIO5_7];	/* P2_8 */
constexpr GPIO gpio_lcd_rdx		= gpio[GPIO5_4];	/* P2_4 */
constexpr GPIO gpio_lcd_wrx		= gpio[GPIO1_10];	/* P2_9 */
constexpr GPIO gpio_dir			= gpio[GPIO1_13];	/* P2_13 */
constexpr std::array<GPIO, 8> gpios_data {
	gpio[GPIO3_8],
	gpio[GPIO3_9],
	gpio[GPIO3_10],
	gpio[GPIO3_11],
	gpio[GPIO3_12],
	gpio[GPIO3_13],
	gpio[GPIO3_14],
	gpio[GPIO3_15],
};

constexpr GPIO gpio_cpld_tms = gpio[GPIO1_1];	// P1_8
constexpr GPIO gpio_cpld_tdo = gpio[GPIO1_8];	// P1_5
constexpr GPIO gpio_cpld_tck = gpio[GPIO3_0];	// P6_1
constexpr GPIO gpio_cpld_tdi = gpio[GPIO3_1];	// P6_2

constexpr auto pin_i2s0_mclk   = pins[CLK2];
constexpr auto pin_i2s0_sck    = pins[P3_0];
constexpr auto pin_i2s0_ws     = pins[P3_1];
constexpr auto pin_i2s0_tx_sda = pins[P3_2];
constexpr auto pin_i2s0_rx_sda = pins[P6_2];

} /* namespace portapack */

#endif/*__PORTAPACK_HAL_H__*/
