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
#include "led.hpp"

#include <array>

using namespace lpc43xx;

namespace hackrf {
namespace one {

/* GPIO */

constexpr GPIO gpio_led_usb = gpio[GPIO2_1];
constexpr GPIO gpio_led_rx = gpio[GPIO2_2];
constexpr GPIO gpio_led_tx = gpio[GPIO2_8];

constexpr GPIO gpio_1v8_enable = gpio[GPIO3_6];
constexpr GPIO gpio_vregmode = gpio[GPIO3_7];
constexpr GPIO gpio_vaa_disable = gpio[GPIO2_9];

constexpr GPIO gpio_rx_mix_bp = gpio[GPIO2_12];
constexpr GPIO gpio_tx_mix_bp = gpio[GPIO2_11];
constexpr GPIO gpio_mix_bypass = gpio[GPIO5_16];
constexpr GPIO gpio_not_mix_bypass = gpio[GPIO1_0];

constexpr GPIO gpio_rx = gpio[GPIO5_5];
constexpr GPIO gpio_tx = gpio[GPIO5_15];

constexpr GPIO gpio_lp = gpio[GPIO2_10];
constexpr GPIO gpio_hp = gpio[GPIO2_0];

constexpr GPIO gpio_rx_amp = gpio[GPIO1_11];
constexpr GPIO gpio_tx_amp = gpio[GPIO2_15];
constexpr GPIO gpio_amp_bypass = gpio[GPIO0_14];
constexpr GPIO gpio_not_rx_amp_pwr = gpio[GPIO1_12];
constexpr GPIO gpio_not_tx_amp_pwr = gpio[GPIO3_5];

constexpr GPIO gpio_rffc5072_resetx = gpio[GPIO2_14];
constexpr GPIO gpio_rffc5072_select = gpio[GPIO2_13];
constexpr GPIO gpio_rffc5072_clock = gpio[GPIO5_6];
constexpr GPIO gpio_rffc5072_data = gpio[GPIO3_3];

constexpr GPIO gpio_max2837_select = gpio[GPIO0_15];
constexpr GPIO gpio_max2837_enable = gpio[GPIO2_6];
constexpr GPIO gpio_max2837_rxenable = gpio[GPIO2_5];
constexpr GPIO gpio_max2837_txenable = gpio[GPIO2_4];

constexpr GPIO gpio_max5864_select = gpio[GPIO2_7];

constexpr GPIO gpio_baseband_invert = gpio[GPIO0_13];

constexpr GPIO gpio_cpld_tdo = gpio[GPIO5_18];
constexpr GPIO gpio_cpld_tck = gpio[GPIO3_0];
constexpr GPIO gpio_cpld_tms = gpio[GPIO3_4];
constexpr GPIO gpio_cpld_tdi = gpio[GPIO3_1];

/* LEDs */

constexpr LED led_usb	{ gpio_led_usb };
constexpr LED led_rx	{ gpio_led_rx };
constexpr LED led_tx	{ gpio_led_tx };

} /* namespace one */
} /* namespace hackrf */

#endif/*__HACKRF_GPIO_H__*/
