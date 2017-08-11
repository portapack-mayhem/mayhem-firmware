/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#define JAMMER_CH_WIDTH 1000000
#define JAMMER_MAX_CH 24

#ifndef __JAMMER_H__
#define __JAMMER_H__

namespace jammer {

typedef struct jammer_range {
	bool enabled;
	int64_t min;
	int64_t max;
} jammer_range_t;

enum JammerType : uint32_t {
	TYPE_FSK = 0,
	TYPE_TONE = 1,
	TYPE_SWEEP = 2
};

} /* namespace jammer */

#endif/*__JAMMER_H__*/
