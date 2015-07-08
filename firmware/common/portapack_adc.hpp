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

#ifndef __PORTAPACK_ADC_H__
#define __PORTAPACK_ADC_H__

#include <cstddef>

namespace portapack {

/* ADC0 */

constexpr size_t adc0_touch_yp_input = 0;
constexpr size_t adc0_touch_yn_input = 2;
constexpr size_t adc0_touch_xp_input = 5;
constexpr size_t adc0_touch_xn_input = 6;

/* ADC1 */

constexpr size_t adc1_rssi_input = 1;

} /* namespace portapack */

#endif/*__PORTAPACK_ADC_H__*/
