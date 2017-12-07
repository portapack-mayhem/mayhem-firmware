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

#include "buffer_exchange.hpp"

BufferExchange* BufferExchange::obj { nullptr };

BufferExchange::BufferExchange(
	CaptureConfig* const config
)	// : config_capture { config }
{
	obj = this;
	// In capture mode, baseband wants empty buffers, app waits for full buffers
	fifo_buffers_for_baseband = config->fifo_buffers_empty;
	fifo_buffers_for_application = config->fifo_buffers_full;
}

BufferExchange::BufferExchange(
	ReplayConfig* const config
)	// : config_replay { config }
{
	obj = this;
	// In replay mode, baseband wants full buffers, app waits for empty buffers
	fifo_buffers_for_baseband = config->fifo_buffers_full;
	fifo_buffers_for_application = config->fifo_buffers_empty;
}

BufferExchange::~BufferExchange() {
	obj = nullptr;
	fifo_buffers_for_baseband = nullptr;
	fifo_buffers_for_application = nullptr;
}

StreamBuffer* BufferExchange::get(FIFO<StreamBuffer*>* fifo) {
	while(true) {
		StreamBuffer* p { nullptr };
		fifo->out(p);
		
		if( p ) {
			return p;
		}

		// Put thread to sleep, woken up by M4 IRQ
		chSysLock();
		thread = chThdSelf();
		chSchGoSleepS(THD_STATE_SUSPENDED);
		chSysUnlock();
	}
}

StreamBuffer* BufferExchange::get_prefill(FIFO<StreamBuffer*>* fifo) {
	StreamBuffer* p { nullptr };
	fifo->out(p);
	
	return p;
}
