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

#ifndef __BASEBAND_THREAD_H__
#define __BASEBAND_THREAD_H__

#include "thread_base.hpp"
#include "message.hpp"
#include "baseband_processor.hpp"

#include <ch.h>

class BasebandThread : public ThreadBase {
public:
	Thread* start(const tprio_t priority);

	void on_message(const Message* const message);
	
	// This getter should die, it's just here to leak information to code that
	// isn't in the right place to begin with.
	baseband::Direction direction() const {
		return baseband::Direction::Receive;
	}

	Thread* thread_main { nullptr };
	Thread* thread_rssi { nullptr };
	BasebandProcessor* baseband_processor { nullptr };

private:
	BasebandConfiguration baseband_configuration;

	void run() override;

	BasebandProcessor* create_processor(const int32_t mode);

	void disable();
	void enable();

	void set_configuration(const BasebandConfiguration& new_configuration);
};

#endif/*__BASEBAND_THREAD_H__*/
