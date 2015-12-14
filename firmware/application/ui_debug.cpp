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

#include "ui_debug.hpp"

#include "ch.h"

#include "radio.hpp"
#include "string_format.hpp"

namespace ui {

/* DebugMemoryView *******************************************************/

DebugMemoryView::DebugMemoryView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_label_m0_free,
		&text_label_m0_free_value,
		&text_label_m0_heap_fragmented_free,
		&text_label_m0_heap_fragmented_free_value,
		&text_label_m0_heap_fragments,
		&text_label_m0_heap_fragments_value,
		&button_done
	} });

	const auto m0_free = chCoreStatus();
	text_label_m0_free_value.set(to_string_dec_uint(m0_free, 5));

	size_t m0_fragmented_free_space = 0;
	const auto m0_fragments = chHeapStatus(NULL, &m0_fragmented_free_space);
	text_label_m0_heap_fragmented_free_value.set(to_string_dec_uint(m0_fragmented_free_space, 5));
	text_label_m0_heap_fragments_value.set(to_string_dec_uint(m0_fragments, 5));

	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void DebugMemoryView::focus() {
	button_done.focus();
}

/* RegistersWidget *******************************************************/

void RegistersWidget::update() {
	set_dirty();
}

void RegistersWidget::paint(Painter& painter) {
	draw_legend(painter);

	draw_values(painter);
}

void RegistersWidget::draw_legend(Painter& painter) {
	for(size_t i=0; i<config.registers_count; i+=config.registers_per_row) {
		const Point offset {
			0, static_cast<Coord>((i / config.registers_per_row) * row_height)
		};

		const auto text = to_string_hex(i, config.legend_length);
		painter.draw_string(
			screen_pos() + offset,
			style(),
			text
		);
	}
}

void RegistersWidget::draw_values(
	Painter& painter
) {
	for(size_t i=0; i<config.registers_count; i++) {
		const Point offset = {
			static_cast<Coord>(config.legend_width() + 8 + (i % config.registers_per_row) * (config.value_width() + 8)),
			static_cast<Coord>((i / config.registers_per_row) * row_height)
		};

		const auto value = reader(i);

		const auto text = to_string_hex(value, config.value_length);
		painter.draw_string(
			screen_pos() + offset,
			style(),
			text
		);
	}
}

/* DebugMenuView *********************************************************/

DebugMenuView::DebugMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "Memory",      [&nav](){ nav.push<DebugMemoryView>(nav); } },
		{ "Radio State", [&nav](){ nav.push<NotImplementedView>(nav); } },
		{ "SD Card",     [&nav](){ nav.push<NotImplementedView>(nav); } },
		{ "RFFC5072",    [&nav](){ nav.push<RegistersView>(
			nav, "RFFC5072", RegistersWidgetConfig { 31, 2, 4, 4 },
			[](const size_t register_number) { return radio::first_if.read(register_number); }
		); } },
		{ "MAX2837",     [&nav](){ nav.push<RegistersView>(
			nav, "MAX2837", RegistersWidgetConfig { 32, 2, 3, 4 },
			[](const size_t register_number) { return radio::second_if.read(register_number); }
		); } },
		{ "Si5351C",     [&nav](){ nav.push<RegistersView>(
			nav, "Si5351C", RegistersWidgetConfig { 96, 2, 2, 8 },
			[](const size_t register_number) { return portapack::clock_generator.read_register(register_number); }
		); } },
		{ "WM8731",      [&nav](){ nav.push<NotImplementedView>(nav); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
