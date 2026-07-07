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

#ifndef __HACKRF_GPIO_H__
#define __HACKRF_GPIO_H__

#include "pins.hpp"

#include <array>

using namespace lpc43xx;

namespace hackrf {
namespace one {

/* GPIO */

#ifdef PRALINE

constexpr GPIO gpio_max2831_ld = gpio[GPIO4_11];  // MAX2831 Lock Detect (P9_6)

#else

constexpr GPIO gpio_max2837_rxenable = gpio[GPIO2_5];
constexpr GPIO gpio_max2837_txenable = gpio[GPIO2_4];
constexpr GPIO gpio_max2839_rxtx = gpio[GPIO2_5];
#endif

#ifdef PRALINE
constexpr GPIO gpio_max5864_select = gpio[GPIO6_30];  // PD_16: PRALINE MAX5864 CS
#else
constexpr GPIO gpio_max5864_select = gpio[GPIO2_7];
#endif

constexpr GPIO gpio_q_invert = gpio[GPIO0_13];

#ifdef PRALINE
/* PRALINE has no HackRF CPLD. These pins are used for RFFC5072 and TX_EN instead.
 * Dummy assignments here allow cpld_update.cpp to compile; the functions
 * that use them are never called on PRALINE. */
constexpr GPIO gpio_cpld_tdo = gpio[GPIO3_0];  // dummy: reuse TCK pin
constexpr GPIO gpio_cpld_tms = gpio[GPIO3_1];  // dummy: reuse TDI pin
#else
constexpr GPIO gpio_cpld_tdo = gpio[GPIO5_18];
constexpr GPIO gpio_cpld_tms = gpio[GPIO3_4];
#endif
constexpr GPIO gpio_cpld_tck = gpio[GPIO3_0];
constexpr GPIO gpio_cpld_tdi = gpio[GPIO3_1];

} /* namespace one */
} /* namespace hackrf */

#endif /*__HACKRF_GPIO_H__*/
