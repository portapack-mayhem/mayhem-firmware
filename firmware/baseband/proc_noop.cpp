/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_noop.hpp"
#include "event_m4.hpp"

#include <cstdint>

void NOOPProcessor::execute(const buffer_c8_t& buffer) {
	for (size_t i = 0; i<buffer.count; i++) {
		buffer.p[i] = {0, 0};
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<NOOPProcessor>() };
	event_dispatcher.run();
	return 0;
}
