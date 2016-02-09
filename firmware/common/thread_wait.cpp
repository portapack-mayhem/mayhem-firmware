/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "thread_wait.hpp"

int ThreadWait::sleep() {
	chSysLock();
	thread_to_wake = chThdSelf();
	chSchGoSleepS(THD_STATE_SUSPENDED);
	const auto result = chThdSelf()->p_u.rdymsg;
	chSysUnlock();
	return result;
}

bool ThreadWait::wake_from_interrupt(const int value) {
	if( thread_to_wake ) {
		thread_to_wake->p_u.rdymsg = value;
		chSchReadyI(thread_to_wake);
		thread_to_wake = nullptr;
		return true;
	} else {
		return false;
	}
}
