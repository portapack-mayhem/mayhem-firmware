/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * copyleft (ɔ) 2023 zxkmm under GPL license
 * Copyright (C) 2023 u-foka
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

#ifndef __TOUCH_ADC_H__
#define __TOUCH_ADC_H__

#include "touch.hpp"

namespace touch {
namespace adc {

void init();
void start();

Samples get();

} /* namespace adc */
} /* namespace touch */

#endif /*__TOUCH_ADC_H__*/
