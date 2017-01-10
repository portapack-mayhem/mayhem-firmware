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

#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#include <cstdint>

#include "ff.h"
#include "signal.hpp"

namespace sd_card {
	
extern FATFS fs;

enum class Status : int32_t {
	IOError = -3,
	MountError = -2,
	ConnectError = -1,
	NotPresent = 0,
	Present = 1,
	Mounted = 2,
};

extern Signal<Status> status_signal;

void poll_inserted();
Status status();

} /* namespace sd_card */

#endif/*__SD_CARD_H__*/
