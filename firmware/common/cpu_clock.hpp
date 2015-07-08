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

#ifndef __CPU_CLOCK_H__
#define __CPU_CLOCK_H__

#include <cstdint>

constexpr uint32_t clock_source_irc_f		=  12000000;
constexpr uint32_t clock_source_gp_clkin	=  20000000;
constexpr uint32_t clock_source_pll1_step_f	= 100000000;
constexpr uint32_t clock_source_pll1_f		= 200000000;

void cpu_clock_max_speed();
void cpu_start_audio_pll();

#endif/*__CPU_CLOCK_H__*/
