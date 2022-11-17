/*
 * Copyright (C) 2022 Belousov Oleg
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

#ifndef __CORDIC_H__
#define __CORDIC_H__

#include <cstdint>
#include <cstddef>
#include "dsp_types.hpp"
#include "cordic_16_tab.hpp"

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif

#define CORDIC_HALF_PI	(M_PI * CORDIC_SCALE / 2)
#define CORDIC_PI		(M_PI * CORDIC_SCALE)

namespace dsp {

void cordic(int32_t theta, int32_t *s, int32_t *c);

}

#endif/*__CORDIC_H__*/
