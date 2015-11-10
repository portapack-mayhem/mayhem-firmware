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

#ifndef __EVENT_H__
#define __EVENT_H__

#include "ch.h"

constexpr auto EVT_MASK_RTC_TICK		= EVENT_MASK(0);
constexpr auto EVT_MASK_LCD_FRAME_SYNC	= EVENT_MASK(1);
constexpr auto EVT_MASK_SWITCHES		= EVENT_MASK(3);
constexpr auto EVT_MASK_ENCODER			= EVENT_MASK(4);
constexpr auto EVT_MASK_TOUCH			= EVENT_MASK(5);
constexpr auto EVT_MASK_APPLICATION		= EVENT_MASK(6);

void events_initialize(Thread* const event_loop_thread);

extern Thread* thread_event_loop;

inline void events_flag(const eventmask_t events) {
	if( thread_event_loop ) {
		chEvtSignal(thread_event_loop, events);
	}
}

inline void events_flag_isr(const eventmask_t events) {
	if( thread_event_loop ) {
		chEvtSignalI(thread_event_loop, events);
	}
}

#endif/*__EVENT_H__*/
