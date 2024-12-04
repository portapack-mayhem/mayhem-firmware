/*
 * Copyright 2018-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2018 Jared Boone
 *
 * This file is part of HackRF.
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

#ifndef __UI_PORTAPACK_H__
#define __UI_PORTAPACK_H__

#include "hackrf_ui.h"

const hackrf_ui_t* portapack_hackrf_ui_init() __attribute__((weak));

#endif /*__UI_PORTAPACK_H__*/
