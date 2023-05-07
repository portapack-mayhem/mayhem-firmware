/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "proc_spectrum_painter.hpp"
// #include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

// This is called at 3072000/2048 = 1500Hz
void SpectrumPainterProcessor::execute(const buffer_c8_t& buffer) {
	
	if (!configured) return;


}

void SpectrumPainterProcessor::on_message(const Message* const msg) {
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<SpectrumPainterProcessor>() };
	event_dispatcher.run();
	return 0;
}
